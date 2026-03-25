#ifndef USB_XHCI_H
#define USB_XHCI_H

#include <stdint.h>

// Capability Register Offsets
#define XHCI_CAPLENGTH    0x00 // Capability Register Length
#define XHCI_HCIVERSION   0x02 // Interface Version Number
#define XHCI_HCSPARAMS1   0x04 // Structural Parameters 1
#define XHCI_HCSPARAMS2   0x08 // Structural Parameters 2
#define XHCI_HCSPARAMS3   0x0C // Structural Parameters 3
#define XHCI_HCCPARAMS1   0x10 // Capability Parameters 1
#define XHCI_DBOFF        0x14 // Doorbell Offset
#define XHCI_RTSOFF       0x18 // Runtime Register Space Offset

// Operational Register Offsets
#define XHCI_USBCMD       0x00 // USB Command
#define XHCI_USBSTS       0x04 // USB Status
#define XHCI_PAGESIZE     0x08 // Page Size
#define XHCI_DNCTRL       0x14 // Device Notification Control
#define XHCI_CRCR         0x18 // Command Ring Control Register (64-bit)
#define XHCI_DCBAAP       0x30 // Device Context Base Address Array Ptr (64-bit)
#define XHCI_CONFIG       0x38 // Configure
#define XHCI_PORTSC_BASE  0x40 // Port Status and Control Registers Base

// USBCMD Register Bits
#define XHCI_CMD_RS       (1 << 0) // Run/Stop
#define XHCI_CMD_HCRST    (1 << 1) // Host Controller Reset
#define XHCI_CMD_INTE     (1 << 2) // Interrupter Enable
#define XHCI_CMD_EWE      (1 << 10)// Enable Wrap Event

// USBSTS Register Bits
#define XHCI_STS_HCH      (1 << 0) // HC Halted
#define XHCI_STS_HSE      (1 << 2) // Host System Error
#define XHCI_STS_EINT     (1 << 3) // Event Interrupt
#define XHCI_STS_CNR      (1 << 11)// Controller Not Ready

// Runtime Register Offsets
#define XHCI_IMAN         0x20
#define XHCI_IMOD         0x24
#define XHCI_ERSTSZ       0x28 // Event Ring Segment Table Size
#define XHCI_ERSTBA       0x30 // Event Ring Segment Table Base Address (64-bit)
#define XHCI_ERDP         0x38 // Event Ring Dequeue Pointer (64-bit)

// TRB (Transfer Request Block) Structure (16 bytes)
typedef struct {
    uint32_t ptr_low;
    uint32_t ptr_high;
    uint32_t status;
    uint32_t control;
} __attribute__((packed)) xhci_trb_t;

// ERST (Event Ring Segment Table) Entry
typedef struct {
    uint32_t seg_base_lo;
    uint32_t seg_base_hi;
    uint32_t size;         // Number of TRBs in the segment
    uint32_t rsvd;
} __attribute__((packed)) xhci_erst_entry_t;

#endif
