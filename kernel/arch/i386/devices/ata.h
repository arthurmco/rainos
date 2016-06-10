/*  ATA disk driver

    Copyright (C) 2016 Arthur M
*/

#include <stddef.h>
#include <stdint.h>
#include <kstdlib.h>

#include "pci.h"
#include "ioport.h"

#ifndef _ATA_H
#define _ATA_H

struct AtaIdentify {
    uint16_t data[256];
};

struct AtaDevice {
    uint16_t iobase;
    uint16_t ioalt; /* IO address of alternate status register */
    uint8_t number; /* 0: Pri, 1: Sec, 2: Third, 3: Fourth... */
    uint8_t slavery; /* 0: Master, 1: Slave */

    struct AtaIdentify* ident;
};

/* ATA IO Ports, called the Task File */
enum AtaRegisters {
    ATAREG_DATAPORT     = 0,
    ATAREG_FEATURES     = 1,
    ATAREG_SECTORCOUNT  = 2,
    ATAREG_LBALOW       = 3,
    ATAREG_LBAMID       = 4,
    ATAREG_LBAHIGH      = 5,
    ATAREG_DRIVESELECT  = 6,
    ATAREG_COMMANDPORT  = 7,
    ATAREG_ALTSTATUS    = 0x206,

    #define ATAREG_STATUS ATAREG_COMMANDPORT
    #define ATAREG_SECTORNUMBER ATAREG_LBALOW
    #define ATAREG_CYLINDERLOW ATAREG_LBAMID
    #define ATAREG_CYLINDERHIGH ATAREG_LBAHIGH
    #define ATAREG_HEAD ATAREG_DRIVESELECT
};

/* ATA Status Definitions */
#define IS_ERR(status)  (status & 0x1)          // Error?
#define IS_DRQ(status)  (status & (1 << 3))     // Data Request?
#define IS_DF(status)   (status & (1 << 5))     // Device Fault?
#define IS_DRDY(status) (status & (1 << 6))     // Device Ready?
#define IS_BSY(status)  (status & (1 << 7))     // Busy?

/* ATA Command Definitions */
enum AtaCommands {
    ATACMD_IDENTIFY = 0xEC,
};

#define MAX_ATA_DEVS 16

/* Check to see if it's a valid ATA device.
    Returns 1 if valid, 0 if not */
int ata_check_device(struct PciDevice* dev);

int ata_initialize(struct PciDevice* dev);

/*  Identify a drive.
    Return 1 if drive is valid, 0 if not
    Automatically fill the 'ident' member of AtaDevice */
int ata_identify(struct AtaDevice* atadev);

void ata_change_drive(struct AtaDevice* atadev);

/* Read/write sectors using PIO */
int ata_read_sector_pio(struct AtaDevice* dev,
    uint64_t lba, size_t count, void* buffer);
int ata_write_sector_pio(struct AtaDevice* dev,
    uint64_t lba, size_t count, void* const buffer);

/* Read/write sectors using DMA */
int ata_read_sector_dma(struct AtaDevice* dev,
    uint64_t lba, size_t count, void* buffer);
int ata_write_sector_dma(struct AtaDevice* dev,
    uint64_t lba, size_t count, void* const buffer);


#endif /* end of include guard: _ATA_H */
