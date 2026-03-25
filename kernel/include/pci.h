#ifndef PCI_H
#define PCI_H
#include <stdint.h>
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC
void     pci_init(void);
uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void     pci_config_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val);
void     usb_host_found(uint8_t bus, uint8_t slot, uint8_t func, uint8_t type);
#endif
