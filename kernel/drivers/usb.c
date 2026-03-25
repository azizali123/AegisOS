#include "../include/usb.h"
#include "../include/pci.h"
#include "../include/vga.h"
#include "../include/io.h"
#include <stdint.h>

void xhci_init(uint32_t bar) {
    if (!bar) return;
    volatile uint8_t *cap = (volatile uint8_t*)(uintptr_t)bar;
    uint8_t cap_len = cap[0];
    volatile uint32_t *op = (volatile uint32_t*)(uintptr_t)(bar + cap_len);
    op[0] |= (1u<<1); /* HCReset */
    int t = 50000; while ((op[0] & (1u<<1)) && t-- > 0) __asm__ volatile("pause");
    vga_puts("    [xHCI] Hazir\n", 0x0A);
}

void ehci_init(uint32_t bar) {
    if (!bar) return;
    volatile uint8_t *cap = (volatile uint8_t*)(uintptr_t)bar;
    uint8_t cap_len = cap[0];
    volatile uint32_t *op = (volatile uint32_t*)(uintptr_t)(bar + cap_len);
    op[0] |= (1u<<1); /* HCReset */
    int t = 50000; while ((op[0] & (1u<<1)) && t-- > 0) __asm__ volatile("pause");
    op[0x40/4] = 1; /* ConfigFlag */
    vga_puts("    [EHCI] Hazir\n", 0x0B);
}

void usb_host_found(uint8_t bus, uint8_t slot, uint8_t func, uint8_t type) {
    uint32_t bar = pci_config_read_dword(bus,slot,func,0x10) & 0xFFFFFFF0;
    uint16_t cmd = pci_config_read_word(bus,slot,func,0x04);
    pci_config_write_dword(bus,slot,func,0x04,(uint32_t)(cmd|0x06));
    switch(type) {
        case 0x00: vga_puts("  [USB] UHCI (USB 1.1)\n",0x07); break;
        case 0x10: vga_puts("  [USB] OHCI (USB 1.1)\n",0x07); break;
        case 0x20: vga_puts("  [USB] EHCI (USB 2.0) BAR=",0x0B); vga_put_hex(bar,0x0F); vga_newline(); ehci_init(bar); break;
        case 0x30: vga_puts("  [USB] xHCI (USB 3.x) BAR=",0x0A); vga_put_hex(bar,0x0F); vga_newline(); xhci_init(bar); break;
        default:   vga_puts("  [USB] Bilinmeyen\n",0x07); break;
    }
}

void usb_init(void) {
    vga_puts("[USB] PCI tarama ile USB yigin baslatiliyor...\n", 0x09);
}
