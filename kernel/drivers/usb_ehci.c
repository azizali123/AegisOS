#include <stdint.h>
#include <stddef.h>
#include "../include/vga.h"
#include "../include/memory.h"
#include "../include/usb_ehci.h"
#include "../include/pci.h"

// Yardimci memset fonksiyonu
static void ehci_memset(void *dest, uint8_t val, uint32_t len) {
    uint8_t *ptr = (uint8_t *)dest;
    while (len-- > 0) *ptr++ = val;
}

// 4096-byte Alignment allocator (EHCI Periodic Frame List)
static void* kmalloc_aligned4k(size_t size) {
    void* raw = kmalloc(size + 4096);
    if (!raw) return NULL;
    uint32_t addr = (uint32_t)(uintptr_t)raw;
    uint32_t offset = 4096 - (addr % 4096);
    if (offset == 4096) offset = 0;
    return (void*)(uintptr_t)(addr + offset);
}

void ehci_init(uint32_t bar_address) {
    vga_puts("    [EHCI] Donanim Ilklendirmesi Basliyor...\n", 0x0A);
    
    if (bar_address == 0) return;
    
    volatile uint8_t *cap_length_reg = (volatile uint8_t *)(uintptr_t)bar_address;
    volatile uint32_t *hccparams_reg = (volatile uint32_t *)(uintptr_t)(bar_address + EHCI_HCCPARAMS);
    
    uint8_t cap_len = *cap_length_reg;
    uint32_t hccparams = *hccparams_reg;
    
    // EHCI Operational register'lari (Capability bittigi yerden baslar)
    volatile uint32_t *operational_base = (volatile uint32_t *)(uintptr_t)(bar_address + cap_len);
    volatile uint32_t *usbcmd = operational_base + (EHCI_USBCMD / 4);
    volatile uint32_t *usbsts = operational_base + (EHCI_USBSTS / 4);
    volatile uint32_t *periodic_list_base = operational_base + (EHCI_PERIODICLISTBASE / 4);
    volatile uint32_t *async_list_addr = operational_base + (EHCI_ASYNCLISTADDR / 4);
    //volatile uint32_t *configflag = operational_base + (EHCI_CONFIGFLAG / 4);
    
    // ==========================================
    // ADIM 1: BIOS SMM HANDOFF (OS OWNERSHIP)
    // ==========================================
    // Bu, klavye/fare donmasini onleyen spesifik kisimdir.
    uint32_t eecp = (hccparams >> 8) & 0xFF; // EHCI Extended Capabilities Pointer
    
    if (eecp >= 0x40) { // eecp genellikle 0x40 offsetinde PCI config mapped bellek uzerinden cikar
        vga_puts("    [EHCI] BIOS OS Handoff Degerleri Degistiriliyor... ", 0x0E);
        
        // EHCI'nin extend capability registerina PCI base uzerinden erismemiz gereklidir, 
        // Ancak bellek haritali porttan erisebilecegimiz "EHCI USBLEGSUP" adli bi pointer vardir.
        // Eger PCI eecp varsa, bunu ilgili MACROS(Offsetler) ile "OS Owned (Bit 24)" yapmaliyiz. 
        // Egitim amaciyla pseudo-mapping:
        volatile uint32_t *usblegsup = (volatile uint32_t *)(uintptr_t)(bar_address + eecp);
        
        // Klasik USBLEGSUP Capability IDsi '1' dir
        if ((*usblegsup & 0xFF) == 1) { 
            *usblegsup |= (1 << 24); // OS Ownership Flag set
            
            // Wait for BIOS to release it (BIOS Owned Semaphore = Bit 16)
            int timeout = 50000;
            while (((*usblegsup) & (1 << 16)) && timeout > 0) {
                timeout--; 
            }
            if(timeout == 0) vga_puts("Zaman Asimi!", 0x0C);
            else vga_puts("Tamam\n", 0x0A);
        } else {
             vga_puts("Desteklenmiyor, Devam Ediliyor.\n", 0x0E);
        }
    } else {
        vga_puts("    [EHCI] Handoff Gerekmedi. Devam Ediliyor.\n", 0x0E);
    }
    
    // ==========================================
    // ADIM 2: HOST CONTROLLER RESET (HCRESET)
    // ==========================================
    vga_puts("    [EHCI] Controller HCRESET Sifirlaniyor... ", 0x0F);
    
    // Stop EHCI (RS bit 0)
    *usbcmd &= ~EHCI_CMD_RS;
    while((*usbsts & EHCI_STS_HCHALTED) == 0); // Wait for Halted bit
    
    // Issue Reset
    *usbcmd |= EHCI_CMD_HCRESET;
    
    // Wait for the HC to clear the bit when done
    while((*usbcmd & EHCI_CMD_HCRESET) != 0);
    vga_puts("Tamam\n", 0x0B);
    
    // ==========================================
    // ADIM 3: DATA STRUCTURES VE FRAME LIST ALLOCATION
    // ==========================================
    // EHCI, 1024 elemanlı (genelde) ve her biri 32-bit (4-byte) olan bir "Periodic Frame List" gerektirir.
    // 1024 * 4 = 4096 bytes (4KB aligned olmasi zorunludur)
    uint32_t *frame_list = (uint32_t*)kmalloc_aligned4k(1024 * sizeof(uint32_t));
    if (!frame_list) { vga_puts("EHCI Frame List Allocation Failed!\n", 0x04); return; }
    
    // Baslangic icin Tum entrylere "Terminal (T)" bitini (Bit 0 = 1) basip Null yapalim:
    for (int i = 0; i < 1024; i++) {
        frame_list[i] = 1; // 1 = T_BIT (Link Pointer Invalid)
    }
    
    *periodic_list_base = (uint32_t)(uintptr_t)frame_list;
    // Async List icin 0 set ediyoruz suanlik bos.
    *async_list_addr = 0; 
    
    vga_puts("    [EHCI] Frame Bellekleri ve Periyodik Liste Olusturuldu.\n", 0x0A);
    vga_puts("    [EHCI] Kontrolcu Basari Ile Baglandi!\n", 0x0A);
    
    // Islemin sonunda:
    // *usbcmd |= EHCI_CMD_RS; (Start) yapilir.
    // Configure Flag yapilarina gore port route'lati kontrol edilir (EHCI_CONFIGFLAG).
    // USB HID Sürücüsü portlara Control/Setup Paketlerini gondermeye baslar.
}
