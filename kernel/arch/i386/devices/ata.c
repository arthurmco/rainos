#include "ata.h"
#include "atapi.h"

#include <kstring.h>

static int atapi_pkt_irq = 0;
int ata_irq(regs_t* registers)
{
    if (atapi_pkt_irq) {
        atapi_pkt_irq = 0;
    }

}

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

int atapi_identify(struct AtaDevice* atadev)
{
    if (inb(atadev->iobase + ATAREG_STATUS) == 0xff) {
        return 0; // No device.
    }

    ata_change_drive(atadev);

    outb(atadev->iobase + ATAREG_SECTORCOUNT,   0x0);
    outb(atadev->iobase + ATAREG_LBALOW,        0x0);
    outb(atadev->iobase + ATAREG_LBAMID,        0x0);
    outb(atadev->iobase + ATAREG_LBAHIGH,       0x0);
    ata_sendcommand(atadev, ATACMD_PKTIDENTIFY);

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

    status = inb(atadev->iobase + ATAREG_STATUS);
    if (IS_ERR(status)) {
        kerror("Error on ATA %s %s",
            (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
            (const char *[]){"Master", "Slave"}[atadev->slavery]);
        return 0;
    }

    status = inb(atadev->iobase + ATAREG_STATUS);
    if (IS_DRQ(status) && !IS_ERR(status)) {
        atadev->ident = kcalloc(sizeof(struct AtaIdentify),1);
        uint16_t* store = (uint16_t*)atadev->ident;
        for (unsigned i = 0; i < 256; i++) {
            store[i] = inw(atadev->iobase + ATAREG_DATAPORT);
            /* swap bytes on texts */
            if (i*2 >= offsetof(struct AtaIdentify, serial_number) &&
                i*2 <= offsetof(struct AtaIdentify, serial_number) + 10) {
                    uint16_t lsb = store[i] & 0xff;
                    uint16_t msb = store[i] >> 8;

                    store[i] = (lsb << 8) | msb;

            }

            if (i*2 >= offsetof(struct AtaIdentify, firmware_rev) &&
                i*2 <= offsetof(struct AtaIdentify, firmware_rev) + 4) {
                    uint16_t lsb = store[i] & 0xff;
                    uint16_t msb = store[i] >> 8;

                    store[i] = (lsb << 8) | msb;

            }

            if (i*2 >= offsetof(struct AtaIdentify, model) &&
                i*2 <= offsetof(struct AtaIdentify, model) + 20) {
                    uint16_t lsb = store[i] & 0xff;
                    uint16_t msb = store[i] >> 8;

                    store[i] = (lsb << 8) | msb;
            }



        }
        return 1;
    }


    return 0;
}

/* Send an ATAPI packet to the device */
int atapi_packet(struct AtaDevice* atadev, uint16_t* packet,
    uint16_t* bytecount, uint16_t* buffer, size_t buffersize)
{
    ata_change_drive(atadev);

    outb(atadev->iobase + ATAREG_FEATURES,      0x0);
    outb(atadev->iobase + ATAREG_LBAMID,        (buffersize & 0xff));
    outb(atadev->iobase + ATAREG_LBAHIGH,       (buffersize >> 8) & 0xff);
    ata_sendcommand(atadev, ATACMD_PACKET);

    uint8_t status;
    /* read port 5 times, so we can be sure status is valid */
    for (unsigned i = 0; i < 5; i++) {
        status = inb(atadev->iobase + ATAREG_STATUS);
    }

    status = inb(atadev->iobase + ATAREG_STATUS);

    uint16_t timeout = 0;

    while (timeout < 0xffff) {
        status = inb(atadev->iobase + ATAREG_STATUS);
        if (IS_DRQ(status) && !IS_BSY(status)) {
            break;
        }

        timeout++;
        io_wait();

        if (IS_ERR(status)) {
            goto media_error;
        }
    }

    if (timeout == 0xffff) {
        kwarn("Timeout while waiting for ATAPI %s %s",
            (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
            (const char *[]){"Master", "Slave"}[atadev->slavery]);
        return 0;
    }

    knotice("atapi: sending packet {%x %x %x %x %x %x}",
        packet[0], packet[1], packet[2], packet[3], packet[4], packet[5]);

    atapi_pkt_irq = 1;

    /* Send the packet data */
    outw(atadev->iobase + ATAREG_DATAPORT, packet[0]);
    outw(atadev->iobase + ATAREG_DATAPORT, packet[1]);
    outw(atadev->iobase + ATAREG_DATAPORT, packet[2]);
    outw(atadev->iobase + ATAREG_DATAPORT, packet[3]);
    outw(atadev->iobase + ATAREG_DATAPORT, packet[4]);
    outw(atadev->iobase + ATAREG_DATAPORT, packet[5]);

    /* Wait IRQ */
    uint16_t wait_sleep = 1;
    while (wait_sleep < 1024) {
        sleep(wait_sleep);
        wait_sleep *= 2;

        if (!atapi_pkt_irq) {
            break;
        }

    }

    if (wait_sleep > 1024) {
        kerror("Timeout while waiting for IRQ on ATAPI %s %s",
            (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
            (const char *[]){"Master", "Slave"}[atadev->slavery]);
        return 0;
    }

    status  = inb(atadev->iobase + ATAREG_STATUS);

    if (IS_ERR(status)) {
        media_error:
        kerror("Error on ATA %s %s ",
            (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
            (const char *[]){"Master", "Slave"}[atadev->slavery]);

        uint8_t err = inb(atadev->iobase + ATAREG_ERR);

        char errs[256] = "";

        if (err & 0x2)  strcat(errs, "No Media, ");
        if (err & 0x4)  strcat(errs, "Command Aborted, ");
        if (err & 0x8)  strcat(errs, "Media Change Request, ");
        if (err & 0x10) strcat(errs, "Sector Beyond Limit, ");
        if (err & 0x40) strcat(errs, "Uncorrectable error, ");
        if (err & 0x20) strcat(errs, "Media Changed, ");

        errs[strlen(errs)-2] = '.';

        kerror("%s (%x) \n", errs, err);

        return 0;
    }

    uint16_t sechi = inb(atadev->iobase + ATAREG_LBAHIGH);
    uint16_t seclo = inb(atadev->iobase + ATAREG_LBAMID);
    knotice("atapi: %d %d", sechi, seclo);

    *bytecount = (sechi << 8) | seclo;

    atapi_pkt_irq = 1;
    for (size_t i = 0; i < (*bytecount / 2); i++) {
        buffer[i] = inw(atadev->iobase + ATAREG_DATAPORT);
        knotice("word: %04x", buffer[i]);
    }

    /* Wait another IRQ */
    wait_sleep = 1;
    while (wait_sleep < 1024) {
        wait_sleep *= 2;
        sleep(wait_sleep);

        if (!atapi_pkt_irq) {
            break;
        }

    }

    if (wait_sleep > 1024) {
        kerror("Timeout while waiting for IRQ on response of command on ATAPI %s %s",
            (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
            (const char *[]){"Master", "Slave"}[atadev->slavery]);
        return 0;
    }

    do {
        status = inb(atadev->iobase + ATAREG_STATUS);
    } while ((IS_BSY(status)) && (IS_DRQ(status)));

    return 1;
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

    uint8_t mid = inb(atadev->iobase + ATAREG_LBAMID);
    atadev->atapi = 0;
    if (mid) {
        if (mid == 0x14) {
            /* truly is 0x01 0x01 0x14 0xeb for sector count and all LBAs */
            knotice("ATAPI device found: %s %s ",
                (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
                (const char *[]){"Master", "Slave"}[atadev->slavery]);
            atadev->atapi = 1;

            return atapi_identify(atadev);

        } else {
            kwarn("ATA device %s %s is not ATA",
                (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
                (const char *[]){"Master", "Slave"}[atadev->slavery]);
        }
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
        uint16_t* store = (uint16_t*)atadev->ident;
        for (unsigned i = 0; i < 256; i++) {
            store[i] = inw(atadev->iobase + ATAREG_DATAPORT);
            /* swap bytes on texts */
            if (i*2 >= offsetof(struct AtaIdentify, serial_number) &&
                i*2 <= offsetof(struct AtaIdentify, serial_number) + 10) {
                    uint16_t lsb = store[i] & 0xff;
                    uint16_t msb = store[i] >> 8;

                    store[i] = (lsb << 8) | msb;

            }

            if (i*2 >= offsetof(struct AtaIdentify, firmware_rev) &&
                i*2 <= offsetof(struct AtaIdentify, firmware_rev) + 4) {
                    uint16_t lsb = store[i] & 0xff;
                    uint16_t msb = store[i] >> 8;

                    store[i] = (lsb << 8) | msb;

            }

            if (i*2 >= offsetof(struct AtaIdentify, model) &&
                i*2 <= offsetof(struct AtaIdentify, model) + 20) {
                    uint16_t lsb = store[i] & 0xff;
                    uint16_t msb = store[i] >> 8;

                    store[i] = (lsb << 8) | msb;
            }



        }
        return 1;
    }


    return 0;
}


static struct AtaDevice devices[MAX_ATA_DEVS];
static unsigned devcount = 0;

static int ata_dev_disk_read_wrapper(disk_t* d,
    uint64_t off, size_t len, void* buf)
{
    if ((unsigned)d->specific >= devcount)
        return 0; // Exceeded the maximum device number

    struct AtaDevice* dev = &devices[d->specific];

    if (dev->atapi) {
        kerror("ata: reads from ATAPI aren't supported");
        return 0;
    }

    if (!dev)
        return 0; // Device is null

    if (!dev->iobase || !dev->ioalt || !dev->ident)
    {
        return 0; // No disk information
    }

    return ata_read_sector_pio(dev, off, len, buf);

}

int ata_initialize(struct PciDevice* dev)
{
    uint16_t base_first =       (uint16_t)(dev->config.bar[0] & ~3);
    uint16_t base_alt_first =   (uint16_t)(dev->config.bar[1] & ~3);
    uint16_t base_second =      (uint16_t)(dev->config.bar[2] & ~3);
    uint16_t base_alt_second =  (uint16_t)(dev->config.bar[3] & ~3);

    /* Add handlers for both IRQs */
    irq_add_handler(14, &ata_irq);
    irq_add_handler(15, &ata_irq);

    /* on problems, default to standard addresses */
    if (base_first == 0)        base_first = 0x1f0;
    if (base_alt_first == 0)    base_alt_first = 0x3f6;
    if (base_second == 0)       base_second = 0x170;
    if (base_alt_second == 0)   base_alt_second = 0x376;

    knotice("ATA: found controller ports %x %x %x %x",
        base_first, base_alt_first, base_second, base_alt_second);
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

                char devname[42];
                memset(devname, 0, 42);
                memcpy(devices[devcount].ident->model, devname, 40);

                for (unsigned i = 39; i > 0; i--) {
                    if (devname[i] != ' ') {
                        devname[i+1] = 0;
                        break;
                    }
                }

                uint64_t disk_size_bytes =
                    ((uint64_t)devices[devcount].ident->sector_count) * 512;

                knotice("\t %s, %d MB", devname,
                    ((uint32_t)(disk_size_bytes / 1048576)) & 0xffffffff);

                disk_t di;
                memset(&di, 0, sizeof(disk_t));
                di.b_count = devices[devcount].ident->sector_count;
                di.b_size = 512;
                di.specific = devcount;
                memcpy(devname, di.disklabel, strlen(devname)+1);

                if (devices[devcount].atapi) {
                    /* Get media size using ATAPI READ CAPACITY command */

                    uint8_t packet[12] = {0x25, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        0x0, 0x0, 0x0, 0x0, 0x0};
                    uint8_t buf[8] = {0,0,0,0,0,0,0,0};
                    uint16_t bsize;
                    atapi_packet(&devices[devcount], (uint16_t*)&packet[0],
                        &bsize, (uint16_t*)&buf[0], 8);

                    uint32_t blsize, blcount;
                    blcount = buf[3];
                    blcount |= ((uint32_t)buf[2] << 8);
                    blcount |= ((uint32_t)buf[1] << 16);
                    blcount |= ((uint32_t)buf[0] << 24);
                    blsize = buf[7];
                    blsize |= ((uint32_t)buf[6] << 8);
                    blsize |= ((uint32_t)buf[5] << 16);
                    blsize |= ((uint32_t)buf[4] << 24);

                    blcount++;

                    knotice("atapi device has %d blocks of %d bytes",
                        blcount, blsize);

                    di.b_count = blcount;
                    di.b_size = blsize;
                }

                if (devices[devcount].atapi) {
                    kerror("ata: read from ATAPI device is not supported");
                    di.__disk_read = &ata_dev_disk_read_wrapper;
                } else {
                    di.__disk_read = &ata_dev_disk_read_wrapper;
                }
                memcpy(devname, di.disk_name, strlen(devname));
                disk_add(&di);

                /* TODO: print disk info to log */
                devcount++;
            }

        }


    }


    knotice("ATA: controller %x:%x ready!",
        dev->config.vendor, dev->config.device);
    return 1;
}

static int ata_wait_ready(struct AtaDevice* atadev)
{
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

        return 0; /* Timeout expired */
    }

    return 1;
}

/* Read/write sectors using PIO */
int ata_read_sector_pio(struct AtaDevice* atadev,
    uint64_t lba, size_t count, void* buffer)
    {
        if (inb(atadev->iobase + ATAREG_STATUS) == 0xff) {
            return 0; // No device.
        }

        ata_change_drive(atadev);

        outb(atadev->iobase + ATAREG_SECTORCOUNT,   count);
        outb(atadev->iobase + ATAREG_LBALOW,        lba & 0xff);
        outb(atadev->iobase + ATAREG_LBAMID,        (lba >> 8) & 0xff);
        outb(atadev->iobase + ATAREG_LBAHIGH,       (lba >> 16) & 0xff);
        outb(atadev->iobase + ATAREG_DRIVESELECT,
                inb(atadev->iobase + ATAREG_DRIVESELECT) | ((lba >> 24) & 0xf) | 1 << 6);
        ata_sendcommand(atadev, ATACMD_READ);

        if (!ata_wait_ready(atadev)) {
            kwarn("Timeout while waiting for ATA %s %s do a PIO read",
                (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
                (const char *[]){"Master", "Slave"}[atadev->slavery]);
        }

        uint8_t status = inb(atadev->iobase + ATAREG_STATUS);
        if (IS_ERR(status) || IS_DF(status)) {
            drive_error:
            kerror("Error on ATA %s %s ",
                (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
                (const char *[]){"Master", "Slave"}[atadev->slavery]);

            uint8_t err = inb(atadev->iobase + ATAREG_ERR);

            char errs[256] = "";

            if (err & 0x2)  strcat(errs, "No Media, ");
            if (err & 0x4)  strcat(errs, "Command Aborted, ");
            if (err & 0x8)  strcat(errs, "Media Change Request, ");
            if (err & 0x10) strcat(errs, "Sector Beyond Limit, ");
            if (err & 0x20) strcat(errs, "Media Changed, ");
            if (err & 0x40) strcat(errs, "Uncorrectable error, ");

            if (IS_DF(status))  strcat(errs, "Drive Fault, ");

            errs[strlen(errs)-2] = '.';

            kerror("%s (%x) \n", errs, err);

            return 0;
        }

        if (IS_DRDY(status)) {
            uint16_t* buf = (uint16_t*)buffer;
            for(uint16_t w = 0; w < (count*256); w++) {

                status = inb(atadev->iobase+ATAREG_STATUS);
                do {
                    status = inb(atadev->iobase+ATAREG_STATUS);
                } while (!IS_DRQ(status));

                if (IS_ERR(status)){
                    goto drive_error;
                }

                buf[w] = inw(atadev->iobase + ATAREG_DATAPORT);
            }
            return 1;
        }

        return 0;

    }
int ata_write_sector_pio(struct AtaDevice* atadev,
    uint64_t lba, size_t count, void* const buffer )
    {
        if (inb(atadev->iobase + ATAREG_STATUS) == 0xff) {
            return 0; // No device.
        }

        ata_change_drive(atadev);

        outb(atadev->iobase + ATAREG_SECTORCOUNT,   count);
        outb(atadev->iobase + ATAREG_LBALOW,        lba & 0xff);
        outb(atadev->iobase + ATAREG_LBAMID,        (lba >> 8) & 0xff);
        outb(atadev->iobase + ATAREG_LBAHIGH,       (lba >> 16) & 0xff);
        outb(atadev->iobase + ATAREG_DRIVESELECT,
                inb(atadev->iobase + ATAREG_DRIVESELECT) | ((lba >> 24) & 0xf) | 1 << 6);
        ata_sendcommand(atadev, ATACMD_WRITE);

        if (!ata_wait_ready(atadev)) {
            kwarn("Timeout while waiting for ATA %s %s do a PIO read",
                (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
                (const char *[]){"Master", "Slave"}[atadev->slavery]);
        }

        uint8_t status = inb(atadev->iobase + ATAREG_STATUS);
        if (IS_ERR(status) || IS_DF(status)) {
            kerror("Error on ATA %s %s",
                (const char *[]){"Pri", "Sec", "Third", "Fourth"}[atadev->number],
                (const char *[]){"Master", "Slave"}[atadev->slavery]);

            uint8_t err = inb(atadev->iobase + ATAREG_ERR);
            char errs[256];

            if (err & 0x2)  strcat(errs, "No Media, ");
            if (err & 0x4)  strcat(errs, "Command Aborted, ");
            if (err & 0x8)  strcat(errs, "Media Change Request, ");
            if (err & 0x10) strcat(errs, "Sector Beyond Limit, ");
            if (err & 0x20) strcat(errs, "Media Changed, ");
            if (err & 0x40) strcat(errs, "Uncorrectable error, ");

            if (IS_DF(status))  strcat(errs, "Drive Fault, ");

            kerror("%s (%x) \n", errs, err);

            return 0;
        }

        if (IS_DRDY(status)) {
            uint16_t* buf = (uint16_t*)buffer;
            for(uint16_t w = 0; w < (count*256); w++) {
                outw(atadev->iobase + ATAREG_DATAPORT, buf[w]);
            }
            return 1;
        }

        return 0;

    }
