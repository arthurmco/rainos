/*  PCI device query support

    Copyright (C) 2016 Arthur M

*/

#include <stdint.h>
#include "ioport.h"

#include <kstdlog.h>

#ifndef _PCI_H
#define _PCI_H

#define MAX_PCI_DEVS 64

union PciConfAddress {
    uint32_t data;
    struct {
        unsigned zero:2;
        unsigned reg:6;
        unsigned function:3;
        unsigned device:5;
        unsigned bus:8;
        unsigned rsvd:7;
        unsigned enable:1;
    } __attribute__((packed)) content;
} __attribute__((packed));

struct PciConfiguration {
    uint16_t vendor, device;
    uint16_t command, status;
    uint8_t rev_id, prog_if, subclass, class_code;
    uint8_t cache_line_size, latency_timer, header_type, bist;
    uint32_t bar[6];
    uint32_t cardbus_cis_ptr;
    uint16_t subsys_vendor_id, subsys_id;
    uint32_t expansion_rom_addr;
    uint8_t capabilities_ptr;
    unsigned rsvd1:24;
    uint32_t rsvd2;
    uint8_t int_line, int_pin, min_grant, max_latency;
} __attribute__((packed));

struct PciDevice {
    uint8_t bus, dev, func;
    struct PciConfiguration config;
} __attribute__((packed));

uint16_t pci_read_conf_data_word(uint8_t bus, uint8_t dev, uint8_t fun,
    uint8_t offset);

/*  Return device by bus/vendor data. Returns 1 if data is valid, 0
    if isn't */
int pci_get_device_bus(uint8_t bus, uint8_t dev, uint8_t fun,
    struct PciDevice* pcidevice);
int pci_get_device_vendor(uint16_t device, uint16_t vendor,
    struct PciDevice* pcidevice);
struct PciDevice* pci_get_device_index(unsigned index);

unsigned pci_get_device_count();
/*  Init PCI subsystem
    Detect devices */
int pci_init();






#endif /* end of include guard: _PCI_H */
