/*  Disk API for the kernel

    Copyright (C) 2016 Arthur M */

#include <kstdlib.h>
#include <kstdlog.h>
#include <kstring.h>

#include "dev.h"

#ifndef _DISK_H
#define _DISK_H

#define MAX_DISKS 10

typedef struct disk {
    char disk_name[64]; // Disk name, reported by IDENTIFY

    int id;
    int specific; //Used by disk driver to identify the disk.
    size_t b_size; // Disk block size
    size_t b_count; // Block count.

    /* Read 'len' blocks, starting at block 'off' at buffer 'buf' */
    int (*__disk_read)(struct disk* d, uint64_t off, size_t len, void* buf);

    /* Write */
    int (*__disk_write)(struct disk* d, uint64_t off, size_t len,
        const void* buf);

    device_t* dev;

    /* Disk system name (disk0, disk1...) */
    char sysname[8];

    /* Disk label */
    char disklabel[40];
} disk_t;

int disk_add(struct disk* d);
void disk_remove(int id);

/* Get a disk by his system name (i.e disk0, disk1) */
disk_t* disk_get(const char* name);

int disk_read(disk_t* d, uint64_t off, size_t len, void* buf);
int disk_write(disk_t* d, uint64_t off, size_t len, const void* buf);

#endif /* end of include guard: _DISK_H */
