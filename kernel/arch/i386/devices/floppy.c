#include "floppy.h"

static volatile int wait_recalibration = 0, wait_on_irq = 0;
static struct floppy_data floppies[MAX_FLOPPIES];

void floppy_halt_motor(struct floppy_data* f)
{
    outb(FLOPPY0_BASE + FREG_DOR,
        inb(FLOPPY0_BASE + FREG_DOR) & ~(0x1 << (4+f->num)));
}

void floppy_start_motor(struct floppy_data* f)
{
    outb(FLOPPY0_BASE + FREG_DOR,
        inb(FLOPPY0_BASE + FREG_DOR) | (0x1 << (4+f->num)));
}

static void floppy_irq6(regs_t* r);

static int dev_floppy_read(device_t* dev, uint64_t off, size_t len, void* buf)
{
    if (dev->devid < 0x820AA && dev->devid > 0x820AF) {
        kerror("floppy: invalid device %s", dev->devname);
        return 0;
    }

    uint8_t sec, cyl, head, seccount;
    struct floppy_data* f = &floppies[(dev->devid-0x820AA) & 3];

    uint16_t buf_off = off % 512;
    uint32_t roff = off / 512;
    knotice("floppy: byte %d = lba %d byte %d", (uint32_t)(off),
        roff, buf_off);

    uint8_t* tmp_buf = kcalloc(512, 1+((len+buf_off)/512));

    LBA2CHS(f, roff, sec, head, cyl);
    seccount = (uint8_t)1+((len / 512) & 0xff);


    knotice("floppy: reading sec=%d cyl=%d head=%d count=%d from lba %d",
        sec, cyl, head, seccount, roff);

    int readcount = (readcount > (f->sectors-sec)) ? (f->sectors-sec) : seccount;
    int count = 0;
    do {
        size_t rcount = (readcount >= (f->sectors-sec+1)) ? (f->sectors-sec+1) : readcount;
        knotice("floppy:  sec=%d cyl=%d head=%d count=%d from lba %d",
            sec, cyl+count, head, rcount, roff);
        int timeout = 5, r;

        while (timeout > 0) {
            r =  floppy_read(f, sec, head, cyl+count, rcount,
                &tmp_buf[count * (f->sectors * 512)]);
            if (r) break;
            timeout--;
            kwarn("floppy read error, retrying more %d times", timeout);
        }

        if (timeout <= 0) return r;

        readcount -= rcount;
        sec = 0;
    } while (readcount > 0);

    knotice("%x %x", tmp_buf[0], tmp_buf[1]);
    memcpy(&tmp_buf[buf_off], buf, len);
    kfree(tmp_buf);
    return 1;
}

static int is_media;            // Set to 1 if there's media on the drive.
static int sector0chksum;       // Sum off all bytes of sector 0.
static int dev_floppy_ioctl(device_t* dev, uint32_t op, uint64_t* ret,
    uint32_t data1,  uint64_t data2)
    {
        /* TODO: Put a lock here */

        if (dev->devid < 0x820AA && dev->devid > 0x820AF) {
            kerror("floppy: invalid device %s", dev->devname);
            return 0;
        }

        struct floppy_data* f = &floppies[(dev->devid-0x820AA) & 3];

        switch (op) {
            case IOCTL_CHANGED:
                floppy_start_motor(f);
                uint8_t dir = inb(FLOPPY0_BASE + FREG_DIR) & 0x80;
                if (!dir) {
                    // No media
                    *ret = 0xffffffff;
                    return 1;
                }

                if (dir) {
                    int oldchksum = sector0chksum;

                    char buf[512];
                    /*  Read the first sector. The checksum will be recalculated
                        and we win */
                    floppy_read(f, 1, 0, 0, 1, buf);

                    if (oldchksum != sector0chksum)
                        *ret = 0x1;

                    return 1;
                }

            default:
                kerror("floppy: unsupported ioctl (%x)", op);
                return 0;
        }
    }

int floppy_init()
{
    /* Read floppy data from CMOS */
    outb(0x70, 0x10);
    irq_add_handler(6, &floppy_irq6);
    uint8_t floppy_data = inb(0x71);

    if (floppy_data == 0) {
        kerror("floppy: none found");
        return 0;
    }

    const char* floppy_data_cmos[] =
        {"none", "360k floppy", "1,2M floppy", "720k floppy",
            "1.44M floppy", "2.88M floppy"};

    knotice("floppy: number 0 is %s, number 1 is %s",
        floppy_data_cmos[floppy_data >> 4],
        floppy_data_cmos[floppy_data & 0xf]);

    /* Shutdown the floppy */
    outb(FLOPPY0_BASE + FREG_DOR, 0x0);

    if ((floppy_data >> 4)) {
        floppies[0].type = (floppy_data >> 4);
        /* Configure first floppy */
        sleep(1);

        switch (floppies[0].type) {
            case FTYPE_144:
                floppies[0].sectors = 18;
                floppies[0].cylinders = 80;
                floppies[0].heads = 2;
            break;
        }
        floppies[0].last_cyl = 0xff;

        /* Enable first driver and DMA/IRQ */
        wait_on_irq = 1;
        outb(FLOPPY0_BASE + FREG_DOR, 0x10 | 0x8 | 0x4);

        uint16_t wait_on_irq_timeout = 2;
        while (wait_on_irq) {
            sleep(wait_on_irq_timeout);
            wait_on_irq_timeout *= 2;

            if (wait_on_irq_timeout >= 4096) {
                kerror("floppy: floppy0 timed out while resetting");
                return 0;
            }
        }

        /* Get version */
        outb(FLOPPY0_BASE + FREG_DATA, FCMD_VERSION);
        do {
            io_wait();
        } while (!(inb(FLOPPY0_BASE + FREG_MSR) & 0x80));

        uint8_t ver = inb(FLOPPY0_BASE + FREG_DATA);
        if (ver != 0x90) {
            kerror("floppy: unsupported controller version: %x", ver);
        } else {
            knotice("floppy: version ok!");
        }

        /* Configure */
        outb(FLOPPY0_BASE + FREG_DATA, FCMD_CONFIGURE);
        io_wait();
        outb(FLOPPY0_BASE + FREG_DATA, 0);
        io_wait();
        outb(FLOPPY0_BASE + FREG_DATA, 0x57); //Polling off, FIFO on, implied seek on, threshold of 8 bytes
        io_wait();
        outb(FLOPPY0_BASE + FREG_DATA, 0); //Default precompensation

        knotice("floppy: floppy0 configured");

        /* Set datarate */
        outb(FLOPPY0_BASE + FREG_DSR, 0); //FIXME: only valid for 1.44m floppies

        /* Select correct drive */
        uint8_t dor = inb(FLOPPY0_BASE + FREG_DOR);
        outb(FLOPPY0_BASE + FREG_DOR, (dor & ~0x3)); //drive 0

        /* Recalibrate */
        wait_recalibration = 1;
        uint8_t recalib_timeout = 0;
        do {
            outb(FLOPPY0_BASE + FREG_DATA, FCMD_RECALIBRATE);
            io_wait();
            outb(FLOPPY0_BASE + FREG_DATA, 0);   // Drive 0

            /* Wait interrupt */
            uint16_t timesleep = 16;
            while (wait_recalibration) {
                sleep(timesleep);
                timesleep *= 2;

                if (timesleep > 4096) {
                    kerror("floppy: floppy0 recalibration failed");
                    return 0;
                }
            }

            uint8_t st0, cyl;
            floppy_sense_int(&floppies[0], &st0, &cyl);

            if (st0 == 0x20) {
                break;
            }

            recalib_timeout++;
        } while (recalib_timeout <= 5);

        if (recalib_timeout > 5) {
            kerror("floppy: floppy0 multiple errors on recalibration");
            return 0;
        }

        knotice("floppy: floppy0 recalibrated");

        /* Set datarate */
        outb(FLOPPY0_BASE + FREG_CCR, 0); //500 kbps

        /* Allocate memory and configure an ISA DMA buffer for the floppy

            This memory will be our 'track pool', we read the whole track
            there and return only the wanted sectors.
            We then cache the track so if the user wants the same track
            we only read from memory
            TODO: do something similar with ATA drives.
        */

        /* 8 pages = 32kb, cover track size for any existant floppy */
        floppies[0].dma_buffer_virt = (void*)vmm_alloc_physical(VMM_AREA_KERNEL,
            &floppies[0].dma_buffer_phys, 8, PMM_REG_LEGACY);

        knotice("floppy: floppy0 dma buffer is at phys 0x%08x, virt 0x%08x",
            floppies[0].dma_buffer_phys, floppies[0].dma_buffer_virt);

        /* Shutdown motor */

        floppy_halt_motor(&floppies[0]);

        outb(0x0a, 0x06);   // mask channel 2
        outb(0xd8, 0xff);      // flip-flop reset

        /* address */
        outb(0x04, floppies[0].dma_buffer_phys & 0xff);
        outb(0x04, (floppies[0].dma_buffer_phys >> 8) & 0xff);

        /* byte count minus one */
        outb(0xd8, 0xff);
        outb(0x05, 0xff);   //0x7fff = 0x8000 - 1 = 32k
        outb(0x05, 0x7f);   //0x7fff = 0x8000 - 1 = 32k
        outb (0x80, 0);     //external page register = 0
        outb(0x0a, 0x02);   // unmask channel 2

        /* Specify config data used by the controller regarding the floppy */
        outb(FLOPPY0_BASE + FREG_DATA, FCMD_SPECIFY);
        /*  seek rate time = 0xA (6ms);
            head load time = 0x5 (10 ms);
            head unload time = 0 (maximum allowed) */
        outb(FLOPPY0_BASE + FREG_DATA, (0xA << 4) | 0x5 );
        outb(FLOPPY0_BASE + FREG_DATA, 0);  // hut = 0, DMA

        floppies[0].num = 0;
        /* Prepare device */
        device_t* fdev = device_create(0x820AA, "floppy0", DEVTYPE_BLOCK, NULL);
        device_set_description(fdev, "Floppy disk drive");
        fdev->b_size = 512;
        fdev->__dev_read = &dev_floppy_read;
        fdev->__dev_ioctl = &dev_floppy_ioctl;
    }

    return 1;
}

static volatile uint8_t irq_io_ok = 0;
static void floppy_irq6(regs_t* r)
{
    if (wait_recalibration == 1) {
        wait_recalibration = 0;
        return;
    }

    if (wait_on_irq == 1) {
        wait_on_irq = 0;
        return;
    }

    if (irq_io_ok == 1) {
        irq_io_ok = 0;
        return;
    }

}

int floppy_read(struct floppy_data* f,
    uint8_t sector, uint8_t head, uint8_t cylinder, uint8_t seccount,
    void* buffer)
    {
        /*** REMEMBER THAT I DIDN'T DO THE TRACK CACHE YET ***/
        if (irq_io_ok) {
            kwarn("floppy0: I/O occurring on floppy%d", f->num);
            do {
                sleep(1000);
            } while (irq_io_ok);
        }

        floppy_start_motor(f);

        irq_io_ok = 1;
        /* select the drive */
        uint8_t dor = inb(FLOPPY0_BASE + FREG_DOR);
        outb(FLOPPY0_BASE + FREG_DOR, (dor & ~0x3) | f->num); //drive 0

        /* setup dma direction */
        outb(0x0a, 0x06);   // mask channel 2
        outb(0x0b, 0x56); //Single DMA, auto reset to initial values, read, channel 2 //
        outb(0x0a, 0x02);   // unmask channel 2

        /* send command */
        outb(FLOPPY0_BASE + FREG_DATA, 0xc0 | FCMD_READ_DATA);
        outb(FLOPPY0_BASE + FREG_DATA, (head << 2) | f->num);
        outb(FLOPPY0_BASE + FREG_DATA, cylinder);
        outb(FLOPPY0_BASE + FREG_DATA, head);
        outb(FLOPPY0_BASE + FREG_DATA, sector);
        outb(FLOPPY0_BASE + FREG_DATA, 2); //512 bytes per sector
        outb(FLOPPY0_BASE + FREG_DATA, seccount);
        outb(FLOPPY0_BASE + FREG_DATA, FLOPPY_GAP1);
        outb(FLOPPY0_BASE + FREG_DATA, 0xff);

        uint32_t sleepamount = 2*seccount;
        /* wait irq */
        do {
            sleep(sleepamount);

            sleepamount *= 2;
            kprintf("%d ", sleepamount);

            if (sleepamount > (2*seccount)*256) {

                kerror("floppy: floppy%d timed out while reading "
                    "sector %d cylinder %d head %d",
                    f->num, sector, cylinder, head);

                // timeout might mean no media
                is_media = 0;

                irq_io_ok = 0;
                floppy_halt_motor(f);
                return 0;
            }
        } while (irq_io_ok);

        uint8_t st0, st1, st2;
        st0 = inb(FLOPPY0_BASE + FREG_DATA);
        st1 = inb(FLOPPY0_BASE + FREG_DATA);
        st2 = inb(FLOPPY0_BASE + FREG_DATA);

        uint8_t rcyl, ehead, esec;
        rcyl = inb(FLOPPY0_BASE + FREG_DATA);
        ehead = inb(FLOPPY0_BASE + FREG_DATA);
        esec = inb(FLOPPY0_BASE + FREG_DATA);

        if (inb(FLOPPY0_BASE + FREG_DATA) != 2) {
            kerror("floppy: error while reading floppy%d "
                "sector %d cylinder %d head %d - ending result value not 2",
                f->num, sector, cylinder, head);
            floppy_halt_motor(f);
            return 0;
        }

        if ((st0 & 0xC0) || st1) {
            kerror("floppy: error while reading floppy%d "
                "sector %d cylinder %d head %d - st0:%x, st1:%x, st2:%x",
                f->num, sector, cylinder, head, st0, st1, st2);
            floppy_halt_motor(f);
            return 0;
        }

        /* if (rcyl != cylinder) {
            kerror("floppy: error while reading floppy%d "
                "sector %d cylinder %d head %d - wrong cylinder",
                f->num, sector, cylinder, head);
            return 0;
        } */

        knotice("floppy: read setup (st0:%x, st1:%x, st2:%x, rcyl:%x, ehead:%x, esec:%x)",
            st0, st1, st2, rcyl, ehead, esec);
        memcpy(f->dma_buffer_virt, buffer, 512*seccount);

        // Take account of that bit in DIR that informs
        // if the drive is opened or closed
        is_media = (inb(FLOPPY0_BASE + FREG_DIR) & 0x80);

        // calculate checksum
        if (sector == 1 && head == 0 && cylinder == 0) {
            uint8_t* b = (uint8_t*)buffer;
            sector0chksum = 0;
            for (size_t i = 0; i < 512; i++)
                sector0chksum += b[i];
        }

        floppy_halt_motor(f);

        return 1;

    }

int floppy_sense_int(struct floppy_data* f, uint8_t* st0, uint8_t* cyl)
{
    outb(FLOPPY0_BASE + FREG_DATA, FCMD_SENSE_INTERRUPT);

    sleep(1);

    *st0 = inb(FLOPPY0_BASE + FREG_DATA);
    *cyl = inb(FLOPPY0_BASE + FREG_DATA);
    knotice("floppy: sense interrupt for floppy%d - st0:%0x, cyl:%x",
        f->num, *st0, *cyl);
}
