/*  Floppy disk driver

    Copyright (C) 2016 Arthur M

    WARNING: This driver is only meant to work in emulators, because no
    computer comes with a floppy driver since the last decade.
*/

#include <stdint.h>
#include <kstdlog.h>
#include <kstdlib.h>
#include "ioport.h"
#include "../irq.h"
#include "../vmm.h"
#include "../../../dev.h"

#ifndef _FLOPPY_H
#define _FLOPPY_H

#define FLOPPY0_BASE 0x3f0
#define MAX_FLOPPIES 2

#define FLOPPY_GAP1 0x1b

enum FloppyRegisters {
    FREG_SRA = 0,    /* Status Register A */
    FREG_SRB = 1,    /* Status Register B */
    FREG_DOR = 2,    /* Digital Output Register */
    FREG_TDR = 3,    /* Tape Drive Register */
    FREG_MSR = 4,    /* Main Status Register (read) */
    FREG_DSR = 4,    /* Datarate Select Register (write) */
    FREG_DATA = 5,   /* Data FIFO (where you write to/read from the floppy) */
    FREG_DIR = 7,    /* Digital Input Register (read) */
    FREG_CCR = 7     /* Configuration Control Register (write) */
};

enum FloppyCommands
{
   FCMD_READ_TRACK =          2, /* generates IRQ6 */
   FCMD_SPECIFY =             3, /* *set drive parameters */
   FCMD_SENSE_DRIVE_STATUS =  4,
   FCMD_WRITE_DATA =          5, /* *write to the disk */
   FCMD_READ_DATA =           6, /* *read from the disk */
   FCMD_RECALIBRATE =         7, /* *seek to cylinder 0 */
   FCMD_SENSE_INTERRUPT =     8, /* *ack IRQ6, get status of last command */
   FCMD_WRITE_DELETED_DATA =  9,
   FCMD_READ_ID =             10,/* generates IRQ6 */
   FCMD_READ_DELETED_DATA =   12,
   FCMD_FORMAT_TRACK =        13,
   FCMD_SEEK =                15,/* *seek both heads to cylinder X */
   FCMD_VERSION =             16,/* *used during initialization, once */
   FCMD_SCAN_EQUAL =          17,
   FCMD_PERPENDICULAR_MODE =  18,/* *used during initialization, once, maybe */
   FCMD_CONFIGURE =           19,/* *set controller parameters */
   FCMD_LOCK =                20,/* *protect controller params from a reset */
   FCMD_VERIFY =              22,
   FCMD_SCAN_LOW_OR_EQUAL =   25,
   FCMD_SCAN_HIGH_OR_EQUAL =  29
};

enum FloppyType {
    FTYPE_144 = 4,
    FTYPE_288 = 5
};

struct floppy_data {
    uint8_t num;
    uint16_t sectors, cylinders, heads, type;
    physaddr_t dma_buffer_phys;
    void* dma_buffer_virt;
    uint8_t last_cyl;
};

#define LBA2CHS(fdata, lba, sec, head, cyl) do { \
    cyl = (lba / (2 * fdata->sectors)); \
    head = ((lba / (fdata->sectors * fdata->cylinders))); \
    sec = ((lba % (2 * fdata->sectors)) % fdata->sectors + 1); \
} while (0); \

/*((lba % (2 * fdata->sectors)) / fdata->sectors); */

int floppy_init();

int floppy_sense_int(struct floppy_data* f, uint8_t* st0, uint8_t* cyl);

int floppy_read(struct floppy_data* f,
    uint8_t sector, uint8_t head, uint8_t cylinder, uint8_t seccount,
    void* buffer);

void floppy_halt_motor(struct floppy_data* f);
void floppy_start_motor(struct floppy_data* f);

#endif /* end of include guard: _FLOPPY_H */
