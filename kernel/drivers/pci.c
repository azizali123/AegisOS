/* ============================================================
   pci.c - PCI Veri Yolu Tarayıcı
   USB + Ses + Depolama cihazlarını tespit eder
   ============================================================ */

#include "../include/pci.h"
#include "../include/io.h"
#include "../include/vga.h"
#include "../include/usb.h"

uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t addr = (1u<<31)|((uint32_t)bus<<16)|((uint32_t)slot<<11)|((uint32_t)func<<8)|(offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, addr);
    return inl(PCI_CONFIG_DATA);
}
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return (uint16_t)((pci_config_read_dword(bus,slot,func,offset) >> ((offset&2)*8)) & 0xFFFF);
}
uint8_t pci_config_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return (uint8_t)((pci_config_read_dword(bus,slot,func,offset) >> ((offset&3)*8)) & 0xFF);
}
void pci_config_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t addr = (1u<<31)|((uint32_t)bus<<16)|((uint32_t)slot<<11)|((uint32_t)func<<8)|(offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, addr);
    outl(PCI_CONFIG_DATA, value);
}

void pci_init(void) {
    vga_puts("[PCI] Veri yolu taranıyor...\n", 0x0F);
    int found = 0;
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            if (pci_config_read_word((uint8_t)bus,(uint8_t)slot,0,0) == 0xFFFF) continue;
            uint8_t hdr = pci_config_read_byte((uint8_t)bus,(uint8_t)slot,0,0x0E);
            int funcs = (hdr & 0x80) ? 8 : 1;
            for (int func = 0; func < funcs; func++) {
                if (pci_config_read_word((uint8_t)bus,(uint8_t)slot,(uint8_t)func,0) == 0xFFFF) continue;
                uint32_t cr = pci_config_read_dword((uint8_t)bus,(uint8_t)slot,(uint8_t)func,0x08);
                uint8_t cls = (uint8_t)((cr>>24)&0xFF);
                uint8_t sub = (uint8_t)((cr>>16)&0xFF);
                uint8_t pi  = (uint8_t)((cr>> 8)&0xFF);
                if (cls == 0x0C && sub == 0x03) { usb_host_found((uint8_t)bus,(uint8_t)slot,(uint8_t)func,pi); found++; }
                if (cls == 0x04 && sub == 0x01)  found++;
            }
        }
    }
    vga_puts("[PCI] Tarama tamamlandi. Bulunan: ", 0x0A);
    vga_put_dec((uint32_t)found, 0x0F);
    vga_newline();
}
