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
    outb(atadev->iobase+ATAREG_DRIVESELECT, (atadev->slavery) ? 0xB0 : 0xA0);
    io_wait();
}

/*  Identify a drive.
    Return 1 if drive is valid, 0 if not
    Automatically fill the 'ident' member of AtaDevice */
int ata_identify(struct AtaDevice* atadev)
{

}


static struct AtaDevice devices[MAX_ATA_DEVS];
static unsigned devcount = 0;
int ata_initialize(struct PciDevice* dev)
{
    uint16_t base_first =       (uint16_t)(dev->config.bar[0] & ~3);
    uint16_t base_alt_first =   (uint16_t)(dev->config.bar[1] & ~3);
    uint16_t base_second =      (uint16_t)(dev->config.bar[2] & ~3);
    uint16_t base_alt_second =  (uint16_t)(dev->config.bar[3] & ~3);

    /* on problems, default to standard addresses */
    if (base_first == 0)        base_first = 0x1f0;
    if (base_alt_first == 0)    base_alt_first = 0x3f6;
    if (base_second == 0)       base_second = 0x170;
    if (base_alt_second == 0)   base_alt_second = 0x376;

    /* 8 is the maximum drives an IDE controller supports
        (2 drives, 4 buses)
     */

    uint16_t base = base_first;
    uint16_t alt = base_alt_first;
    for (unsigned b = 0; b < 4; b++) {
        for (unsigned d = 0; d < 2; d++ ) {
            devices[devcount].iobase = base;
            devices[devcount].ioalt = alt;
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

        if (b == 2) {
            base = base_second;
            alt = base_alt_second;
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
