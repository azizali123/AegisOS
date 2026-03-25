#ifndef USB_EHCI_H
#define USB_EHCI_H

#include <stdint.h>

// EHCI Capability Registers
#define EHCI_CAPLENGTH 0x00
#define EHCI_HCIVERSION 0x02
#define EHCI_HCSPARAMS 0x04
#define EHCI_HCCPARAMS 0x08

// EHCI Operational Registers
#define EHCI_USBCMD 0x00
#define EHCI_USBSTS 0x04
#define EHCI_USBINTR 0x08
#define EHCI_FRINDEX 0x0C
#define EHCI_CTRLDSSEGMENT 0x10
#define EHCI_PERIODICLISTBASE 0x14
#define EHCI_ASYNCLISTADDR 0x18
#define EHCI_CONFIGFLAG 0x40
#define EHCI_PORTSC 0x44

// USBCMD Bits
#define EHCI_CMD_RS (1 << 0)      // Run/Stop
#define EHCI_CMD_HCRESET (1 << 1) // Host Controller Reset
#define EHCI_CMD_PSE (1 << 4)     // Periodic Schedule Enable
#define EHCI_CMD_ASE (1 << 5)     // Asynchronous Schedule Enable
#define EHCI_CMD_IAAD (1 << 6)    // Interrupt on Async Advance Doorbell

// USBSTS Bits
#define EHCI_STS_USBINT (1 << 0)
#define EHCI_STS_ERROR (1 << 1)
#define EHCI_STS_PORT (1 << 2)
#define EHCI_STS_FLR (1 << 3)
#define EHCI_STS_SYSERR (1 << 4)
#define EHCI_STS_ASVD (1 << 5)
#define EHCI_STS_HCHALTED (1 << 12)

#endif
