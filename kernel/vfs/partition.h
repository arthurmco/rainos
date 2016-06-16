/*  Base MBR partition driver parser

    Copyright (C) 2016 Arthur M.
*/

// TODO: Make this a interface and implement GPT (and maybe Apple Part. Map?)

#include <kstring.h>
#include <kstdlog.h>

#include "../dev.h"

#ifndef _PARTITION_H
#define _PARTITION_H

#define MAX_DEVICES 16
#define MAX_PARTITIONS 4

struct partition {
    uint8_t bootable;   /* if 0x80 is bootable */
    unsigned chs_unneeded_shit:24;
    uint8_t system_id;  /* Partition type ID */
    unsigned chs_more_unneeded_shit:24;

    uint32_t lba_start;
    uint32_t lba_size;
} __attribute__((packed));

/* Initialize partition parsing system */
void partitions_init();

/*  Retrieve partitions from a device.
    It needs to be a block device,
        (maybe in the future support char device w/ seeking)

    Returns the count of partitions, or -1 if error

        The partitions will be created at [device_name]p[partition_num],
    staring at 1
        Ex: If we pass disk0 as the device, this function will create
    disk0p1, disk0p2, disk0p3...
*/
int partitions_retrieve(device_t* dev);






#endif /* end of include guard: _PARTITION_H */
