#include "ata.h"

/* Check to see if it's a valid ATA device.
    Returns 1 if valid*/
int ata_check_device(struct PciDevice* dev)
{
    /* class 0x01 = storage device, subclass 0x01 = IDE compatible */
    if (dev->config.class_code == 0x01 &&
        dev->config.subclass == 0x01) {
            return 1;
        } else {
            return 0;
        }
}


void ata_change_drive(struct AtaDevice* atadev)
{
    io_wait();
    uint8_t val = (atadev->slavery) ? 0xB0 : 0xA0;
    outb(atadev->iobase + ATAREG_DRIVESELECT, val);
    io_wait();
}

static void ata_sendcommand(struct AtaDevice* dev, uint8_t cmd) {
    outb(dev->iobase + ATAREG_COMMANDPORT,  cmd);
    io_wait();
}

/*  Identify a drive.
    Return 1 if drive is valid, 0 if not
    Automatically fill the 'ident' member of AtaDevice */
int ata_identify(struct AtaDevice* atadev)
{
    if (inb(atadev->iobase + ATAREG_STATUS) == 0xff) {
        return 0; // No device.
    }

    ata_change_drive(atadev);

    outb(atadev->iobase + ATAREG_SECTORCOUNT,   0x0);
    outb(atadev->iobase + ATAREG_LBALOW,        0x0);
    outb(atadev->iobase + ATAREG_LBAMID,        0x0);
    outb(atadev->iobase + ATAREG_LBAHIGH,       0x0);
    ata_sendcommand(atadev, ATACMD_IDENTIFY);

    uint8_t status;
    /* read port 5 times, so we can be sure status is valid */
    for (unsigned i = 0; i < 5; i++) {
        status = inb(atadev->iobase + ATAREG_STATUS);
    }

    status = inb(atadev->iobase + ATAREG_STATUS);

    if (!status) {

        return 0; // No drive
    }

    unsigned timeout_out = 0;
    for (unsigned i = 0; i < 16; i++) {
        status = inb(atadev->iobase + ATAREG_STATUS);
        if (!IS_BSY(status)) {
            timeout_out = 1;
            break;
        }
        io_wait();
    }

    if (!timeout_out) {
        kwarn("Timeout while waiting for ATA %s %s",
            (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
            (const char *[]){"Master", "Slave"}[atadev->slavery]);

        return 0; /* Timeout expired */
    }

    if (inb(atadev->iobase + ATAREG_LBAMID)) {
        kwarn("ATA device %s %s is not ATA",
            (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
            (const char *[]){"Master", "Slave"}[atadev->slavery]);

        return 0; // Non ata
    }

    status = inb(atadev->iobase + ATAREG_STATUS);
    if (IS_ERR(status)) {
        kerror("Error on ATA %s %s",
            (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
            (const char *[]){"Master", "Slave"}[atadev->slavery]);
        return 0;
    }

    status = inb(atadev->iobase + ATAREG_STATUS);
    if (IS_DRDY(status) && !IS_ERR(status)) {
        atadev->ident = kcalloc(sizeof(struct AtaIdentify),1);
        knotice("<< %x %d >>", atadev->ident, sizeof(struct AtaIdentify));
        uint16_t* store = (uint16_t*)atadev->ident;
        for (int i = 0; i < 256; i++) {
            store[i] = inw(atadev->iobase + ATAREG_DATAPORT);
        }
        return 1;
    }


    return 0;
}


static struct AtaDevice devices[MAX_ATA_DEVS];
static unsigned devcount = 0;
int ata_initialize(struct PciDevice* dev)
{
    uint16_t base_first =       (uint16_t)(dev->config.bar[0] & ~3);
    uint16_t base_alt_first =   (uint16_t)(dev->config.bar[1] & ~3);
    uint16_t base_second =      (uint16_t)(dev->config.bar[2] & ~3);
    uint16_t base_alt_second =  (uint16_t)(dev->config.bar[3] & ~3);

    knotice("ATA: found controller ports %x %x %x %x",
        base_first, base_alt_first, base_second, base_alt_second);
    /* on problems, default to standard addresses */
    if (base_first == 0)        base_first = 0x1f0;
    if (base_alt_first == 0)    base_alt_first = 0x3f6;
    if (base_second == 0)       base_second = 0x170;
    if (base_alt_second == 0)   base_alt_second = 0x376;

    /* 4 is the maximum drives an IDE controller supports
        (2 drives, 4 buses)
     */

    uint16_t bases[2] = {base_first, base_second};
    uint16_t alts[2] = {base_alt_first, base_alt_second};
    for (unsigned b = 0; b < 2; b++) {

        for (unsigned d = 0; d < 2; d++ ) {
            devices[devcount].iobase = bases[b];
            devices[devcount].ioalt = alts[b];
            devices[devcount].number = b;
            devices[devcount].slavery = d;

            if (ata_identify(&devices[devcount])) {
                knotice("ATA: Found device on ATA %s %s",
                    (const char *[]){"Pri", "Sec", "Third", "Fourth"}[b],
                    (const char *[]){"Master", "Slave"}[d]);

                /* TODO: print disk info to log */
                devcount++;
            }

        }

    }


    knotice("ATA: controller %x:%x ready!",
        dev->config.vendor, dev->config.device);
    return 1;
}

/* Read/write sectors using PIO */
int ata_read_sector_pio(struct AtaDevice* dev,
    uint64_t lba, size_t count, void* buffer)
    {

    }
int ata_write_sector_pio(struct AtaDevice* dev,
    uint64_t lba, size_t count, void* const buffer )
    {

    }
