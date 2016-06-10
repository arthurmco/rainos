#include "pci.h"

uint16_t pci_read_conf_data_word(uint8_t bus, uint8_t dev, uint8_t fun,
    uint8_t offset)
    {
        union PciConfAddress confaddr;
        confaddr.content.bus = bus & 0xff;
        confaddr.content.device = dev & 0x1f;
        confaddr.content.function = fun & 0x07;
        confaddr.content.reg = (offset >> 2) & 0x7f;
        confaddr.content.enable = 1;
        confaddr.content.zero = 0;

        /* write it to the PCI config address register */
        outl(0xcf8, confaddr.data);

        /* return the corresponding word */
        uint16_t w = (uint16_t)( inl(0xcfc) >> ((offset & 0x2) * 8) & 0xffff);
        return w;
    }

static struct PciDevice devices_data[MAX_PCI_DEVS];
static size_t devices_count = 0;

/*  Return device by bus/vendor data. Returns 1 if data is valid, 0
    if isn't */
int pci_get_device_bus(uint8_t bus, uint8_t dev, uint8_t fun,
    struct PciDevice* pcidevice)
{
    uint16_t* num = (uint16_t*)&(pcidevice->config);

    for (int i = 0; i < sizeof(struct PciConfiguration)/2; i++) {
        num[i] = pci_read_conf_data_word(bus, dev, fun, i*2);

        if (i == 0) {
            /* we can check if vendor is valid */
            if (pcidevice->config.vendor == 0xffff) {
                // it isn't
                return 0;
            }
        } else if (i == 1) {
            /* we can check if device is valid */
            if (pcidevice->config.device == 0xffff) {
                // it isn't
                return 0;
            }
        }

    }

    pcidevice->bus = bus;
    pcidevice->dev = dev;
    pcidevice->func = fun;

    return 1;
}
int pci_get_device_vendor(uint16_t device, uint16_t vendor,
    struct PciDevice* pcidevice)
{
    for (size_t i = 0; i < devices_count; i++) {
        if (devices_data[i].config.vendor == vendor) {
            if (devices_data[i].config.device == device) {
                pcidevice = &devices_data[i];
                return 1;
            }
        }
    }

    return 0;
}

struct PciDevice* pci_get_device_index(unsigned index)
{
    if (index < devices_count)
        return &devices_data[index];
    else
        return NULL;
}

unsigned pci_get_device_count()
{
    return devices_count;
}

/*  Init PCI subsystem
    Detect devices */
int pci_init()
{
    unsigned max_bus = 2;
    for (unsigned bus = 0; bus < max_bus; bus++) {
        for (unsigned dev = 0; dev < 0x20; dev++) {
            for (unsigned fun = 0; fun < 0x8; fun++) {
                struct PciDevice pcidev;
                if (pci_get_device_bus(bus, dev, fun, &pcidev)) {

                    knotice("PCI: found device at %d:%d.%d", bus, dev, fun);
                    knotice("\t vendor: 0x%x, device: 0x%x, class: %x:%x",
                        pcidev.config.vendor, pcidev.config.device,
                        pcidev.config.class_code, pcidev.config.subclass);
                    knotice("\t header: 0x%x", pcidev.config.header_type);

                    if ((pcidev.config.header_type & 0x7f) == 0) {
                        for (int i = 0; i < 6; i++) {
                            knotice("\t BAR%d: 0x%x", i, pcidev.config.bar[i]);
                        }

                        knotice("\t Interrupt %d, pin %d",
                            pcidev.config.int_line, pcidev.config.int_pin);
                    }

                    knotice("");
                    devices_data[devices_count++] = pcidev;

                    /* check if multifunction */
                    if (pcidev.config.header_type & 0x80) {

                        // if bus 0 and device 0, this is the PCI controller
                        // each function of the PCI controller is a bus.
                        // so we increment by one if we find a controller
                        if (bus == 0 && dev == 0) {
                            max_bus++;
                        }

                    } else {
                        break;
                    }

                }
            }
        }
    }

    knotice("PCI: %d devices detected", devices_count);

    return 1;
}
