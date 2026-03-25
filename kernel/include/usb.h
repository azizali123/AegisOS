#ifndef USB_H
#define USB_H
#include <stdint.h>
#define USB_UHCI 0x00
#define USB_OHCI 0x10
#define USB_EHCI 0x20
#define USB_XHCI 0x30
void usb_init(void);
void usb_host_found(uint8_t bus, uint8_t slot, uint8_t func, uint8_t type);
#endif
