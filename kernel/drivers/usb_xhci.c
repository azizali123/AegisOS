#include <stdint.h>
#include <stddef.h>
#include "../include/vga.h"
#include "../include/memory.h"
#include "../include/usb_xhci.h"
#include "../include/pci.h"

// xHCI driver gereksinimleri icin hizli memset
static void xhci_memset(void *dest, uint8_t val, uint32_t len) {
    uint8_t *ptr = (uint8_t *)dest;
    while (len-- > 0) *ptr++ = val;
}

// 64-byte Alignment (xHCI DCBAA, Ring'ler icin sarti vardir)
static void* kmalloc_aligned64(size_t size) {
    // Allocation + padding. alignment'tan artan olursa 64 kata duzlenmesi saglanir.
    // memory.c sadece basit bir bump-allocator'dir, biz fazladan yer alip hizalari manuel yapariz
    void* raw = kmalloc(size + 64);
    if (!raw) return NULL;
    uint32_t addr = (uint32_t)(uintptr_t)raw;
    uint32_t offset = 64 - (addr % 64);
    if (offset == 64) offset = 0; // zaten hizali
    return (void*)(uintptr_t)(addr + offset);
}

// 4KB Alignment (ERST vs. gerektirebilir)
static void* kmalloc_aligned4k(size_t size) {
    void* raw = kmalloc(size + 4096);
    if (!raw) return NULL;
    uint32_t addr = (uint32_t)(uintptr_t)raw;
    uint32_t offset = 4096 - (addr % 4096);
    if (offset == 4096) offset = 0;
    return (void*)(uintptr_t)(addr + offset);
}

void xhci_init(uint32_t bar_address) {
    vga_puts("    [xHCI] xHCI Dongusu ve Bellegi Hazirlaniyor...\n", 0x0A);
    
    if (bar_address == 0) return;

    volatile uint8_t *cap_base = (volatile uint8_t *)(uintptr_t)bar_address;
    uint8_t cap_len = cap_base[XHCI_CAPLENGTH];
    
    // Yapisal Pointer'lar (Memory Mapped IO - MMIO)
    volatile uint32_t *hcsparams1 = (volatile uint32_t *)(uintptr_t)(bar_address + XHCI_HCSPARAMS1);
    volatile uint32_t *hccparams1 = (volatile uint32_t *)(uintptr_t)(bar_address + XHCI_HCCPARAMS1);
    
    volatile uint32_t *operational_base = (volatile uint32_t *)(uintptr_t)(bar_address + cap_len);
    volatile uint32_t *usbcmd = operational_base + (XHCI_USBCMD / 4);
    volatile uint32_t *usbsts = operational_base + (XHCI_USBSTS / 4);
    // 64 Bit registers (2 adet 32-bit register gibi okuyoruz - 32 bit OS icin)
    volatile uint32_t *dcbaap_low  = operational_base + (XHCI_DCBAAP / 4);
    volatile uint32_t *dcbaap_high = operational_base + ((XHCI_DCBAAP+4) / 4);
    volatile uint32_t *crcr_low    = operational_base + (XHCI_CRCR / 4);
    volatile uint32_t *crcr_high   = operational_base + ((XHCI_CRCR+4) / 4);
    volatile uint32_t *config      = operational_base + (XHCI_CONFIG / 4);
    
    // Donanim parametrelerini oku
    uint32_t hcs1 = *hcsparams1;
    uint8_t max_slots = hcs1 & 0xFF;         // En Fazla Aygit (Device) Sayisi
    uint16_t max_ports = (hcs1 >> 24) & 0xFF;// En Fazla Port Sayisi
    
    // ==========================================
    // ADIM 1: BIOS SMM HANDOFF (OS OWNERSHIP)
    // ==========================================
    // xHCI Extended Capabilities Traverse (Dolayisla HP BIOS'un Klavye Donmasini Engelleme)
    uint32_t hcc1 = *hccparams1;
    uint16_t xecp = (hcc1 >> 16) & 0xFFFF; // Extended Capabilities Pointer Offset
    
    if (xecp != 0) {
        vga_puts("    [xHCI] BIOS'tan OS Kontrolu devraliniyor (Handoff)... ", 0x0E);
        // xecp (Word olarak deger doner. xecp x 4 = Byte Offset)
        uint32_t ext_cap_ptr = (xecp << 2);
        volatile uint32_t *ext_cap = (volatile uint32_t *)(uintptr_t)(bar_address + ext_cap_ptr);
        
        uint32_t id = (*ext_cap) & 0xFF; // Capability ID
        if (id == 1) { // 1 = USB Legacy Support (BIOS)
            // OS Owned Semaphore Flag = Bit 24
            *ext_cap |= (1 << 24); // OS artik benimdiyor
            
            // BIOS'un bu duruma iznini beklemesi (BIOS Owned Flag = Bit 16 sifirlanmali)
            int timeout = 100000;
            while (((*ext_cap) & (1 << 16)) && timeout > 0) {
                // Burada BIOS kendi elini ceker (SMM modunu kapatir).
                // iste klavyenin bir anda Pirt yapip bozulan kismi tam da bu adimdir!
                // Biz bunu bilerek yapiyoruz ki USB 3.0 sürücümüz artık USB klavyeyi yonetebilsin.
                timeout--; 
            }
            if (timeout == 0) vga_puts("Zaman Asimi!", 0x0C);
            vga_puts("Tamam\n", 0x0A);
        } else {
             vga_puts("Desteklenmiyor\n", 0x0E);
        }
    }
    
    // ==========================================
    // ADIM 2: RESET HOST CONTROLLER (HCRST)
    // ==========================================
    vga_puts("    [xHCI] HCRST Komutu ve Kontrolcu Sifirlama... ", 0x0F);
    
    // Controller stop loop (Run/Stop biti sifirlanmali)
    *usbcmd &= ~XHCI_CMD_RS;
    while((*usbsts & XHCI_STS_HCH) == 0); // Halted durumuna gecmesini bekle
    
    // HCRST calistir
    *usbcmd |= XHCI_CMD_HCRST;
    // Reset bitinin donanim tarafindan otomatik temizlenmesini bekle (tamamlaninca)
    while((*usbcmd & XHCI_CMD_HCRST) != 0); 
    
    // CNR (Controller Not Ready) temizlenmesini bekle
    while((*usbsts & XHCI_STS_CNR) != 0);
    vga_puts("Tamam\n", 0x0B);
    
    // ==========================================
    // ADIM 3: MAX DEVICE SLOTS, DCBAA, COMMAND RING
    // ==========================================
    
    // Aygit Slot'larini etkinlestir. CONFIG [0:7] eger Max Device Slots 255'ten buyuk degilse ona ayarlanir.
    *config = max_slots;
    vga_puts("    [xHCI] Aygit Yuvalari Ayarlandi. Yuva Sayisi: ", 0x0B);
    // [Gosterim basitlesildi, aslinda ekrana max_slots decimal yazilmali]

    // DCBAA (Device Context Base Address Array Allocation) (MaxSlots + 1 kadar pointer icerir)
    // 64-byte aligned and contiguous 1024 / 2048 bytes
    uint64_t *dcbaa = (uint64_t *)kmalloc_aligned64( (max_slots + 1) * sizeof(uint64_t) );
    if (!dcbaa) { vga_puts("DCBAA Allocation Failed!\n", 0x04); return; }
    xhci_memset(dcbaa, 0, (max_slots + 1) * sizeof(uint64_t)); // Clear all
    
    // Scratchpad Bufferlar da burada DCBAA[0]'ye ayarlanir (Intel xHCI spec bolum 4.20)
    // Atliyoruz, eger Spec sarti varsa yapilmali, basitlesildi
    
    // DCBAA registerina yaz
    *dcbaap_low = (uint32_t)(uintptr_t)dcbaa;
    *dcbaap_high = 0; // 32 bit isletim sistemi oldugumuz icin ust 32-bit hep 0'dir
    
    // ---------------------------------
    // COMMAND RING OLUSTURMA (Command Ring Buffer)
    // ---------------------------------
    // 256 segment(TRB) x 16 byte = 4096 bytes / 4KB aligned is super safe
    xhci_trb_t *cmd_ring = (xhci_trb_t *)kmalloc_aligned4k(256 * sizeof(xhci_trb_t));
    xhci_memset(cmd_ring, 0, 256 * sizeof(xhci_trb_t));
    
    // CRCR Pointer yazma + Ring Cycle State biti ayari (BIT 0: RCS = 1)
    *crcr_low = ((uint32_t)(uintptr_t)cmd_ring) | 1; 
    *crcr_high = 0;
    
    vga_puts("\n    [xHCI] Ring Bellek Tahsisleri Basarili.\n", 0x0A);
    vga_puts("    [xHCI] USB 3.0 Baslangic Asamasi Kapatildi.\n", 0x0E);
    // Bundan sonra: 
    // - ERST (Event Ring Segment Table) tahsis edilir.
    // - Run/Stop (RS=1) verilerek kontrolcu gercekten calistirilir.
    // - xHCI Portlari "Polling" veya "Kesmeler (INT)" uzerinden statusunu kontrol ederek bagli USB var mi diye bakar.
    // - Portlarda bir tus klavye takilirsa, Command Ring'e "Address Device Command TRB" konulup Doorbell'a basilir.
}
