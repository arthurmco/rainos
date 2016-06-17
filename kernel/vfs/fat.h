/*  FAT filesystem driver for Rain OS

    Copyright (C) 2016 Arthur M
*/

#include "vfs.h"

#include <kstring.h>
#include <kstdlib.h>
#include <kstdlog.h>

#ifndef _FAT_H
#define _FAT_H

struct fat_superblock {
    /* JMP instruction.

        This is for jump through the boot code, so it won't be executed
        acidentally */
    uint8_t jmpBoot[3];

    char oem_name[8];

    uint16_t bytes_sec;
    uint8_t sec_clus;
    uint16_t rsvd_secs;
    uint8_t num_fats;

    /* Number of entries in root directory
        For FAT32, this is set to 0
        For FAT12 and FAT16, this is usually 512. */
    uint16_t root_dir_entries;

    uint16_t tot_sec_16;
    uint8_t media_type; /* __attribute__((useless_shit)) */
    uint16_t fat_size_16;
    uint16_t secs_track;
    uint16_t num_heads;
    uint32_t hidden_secs;
    uint32_t tot_sec_32;

    union _fattype {
        struct _fat16 {
            uint8_t drive_num;
            uint8_t rsvd; //documentation says winnt use this for illuminati things
            uint8_t boot_sig; //Should be 0x29
            uint32_t serial_number;
            char vol_label[11];
            char fstype[8];
        } __attribute__((packed)) f16;
        struct _fat32 {
            uint32_t fat_size_32;
            uint16_t ext_flags;
            uint16_t fat_version;

            /* Cluster of root directory. This is usually 2 */
            uint32_t root_cluster;

            uint16_t fsinfo;
            uint16_t backup_boot_sec;
            char rsvd[12];

            uint8_t drive_num;
            uint8_t rsvd2; //documentation says winnt use this for illuminati things
            uint8_t boot_sig; //Should be 0x29
            uint32_t serial_number;
            char vol_label[11];
            char fstype[8];
        } __attribute__((packed)) f32;
    } fattype;

} __attribute__((packed));

struct fat_dir {
    char name[11];
    uint8_t attr;
    uint8_t nt_illuminati_value;

    uint8_t create_stamp;   //File timestamp, in tenths of a second
    uint16_t create_time;    //File creation time, in FAT format
    uint16_t create_date;    //File creation date, in FAT format
    uint16_t last_acc_date;  //File last access date

    uint16_t cluster_high;

    uint16_t write_time;
    uint16_t write_date;

    uint16_t cluster_low;

    uint32_t size;
} __attribute__((packed));


enum FatDirAttributes
{
FAT_ATTR_READ_ONLY  = 0x01,
FAT_ATTR_HIDDEN 	= 0x02,
FAT_ATTR_SYSTEM     = 0x04,
FAT_ATTR_VOLUME_ID  = 0x08,
FAT_ATTR_DIRECTORY  = 0x10,
FAT_ATTR_ARCHIVE    = 0x20,
FAT_ATTR_LONG_NAME  =
    FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID

};

#define FAT_GET_SECTORS(sb) \
    ((sb->tot_sec_16) ? sb->tot_sec_16 : sb->tot_sec_32)

#define FAT_GET_FAT_SIZE(sb) \
    ((sb->fat_size_16) ? sb->fat_size_16 : sb->fattype.f32.fat_size_32)

/* Get the size of the root directory, in sectors */
#define FAT_GET_ROOT_SECTOR_COUNT(sb) \
    (((sb->root_dir_entries * 32) + (sb->bytes_sec - 1)) / sb->bytes_sec)

/*  Get the sector where the root directory is
    This isn't needed on FAT32 */
#define FAT_GET_ROOT_SECTOR_START(sb) \
    (sb->rsvd_secs + (sb->num_fats * FAT_GET_FAT_SIZE(sb)))

/* Get the first sector that contain data, after the root dir */
#define FAT_GET_FIRST_DATA_SECTOR(sb) \
    (FAT_GET_ROOT_SECTOR_START(sb) + FAT_GET_ROOT_SECTOR_COUNT(sb))

/*  Get the first sector of the cluster */
#define FAT_GET_FIRST_SECTOR_CLUSTER(sb, cluster) \
    (((cluster - 2) * sb->sec_clus) + FAT_GET_FIRST_DATA_SECTOR(sb))

struct fat_fs {
    device_t* dev;
    struct fat_superblock* sb;
    uint8_t fat_type; //12, 16 or 32.
};

void fat_init();

int fat_mount(device_t*);
int fat_get_root_dir(device_t* dev, vfs_node_t** root_childs);

uint8_t fat_get_type(struct fat_superblock* sb);

/* Get the cluster entry offset for a specific cluster in the FAT */
void fat_get_fat_cluster_entry(struct fat_superblock* sb, uint32_t cluster,
    /*[out]*/ uint32_t* sector, /*[out]*/ uint32_t* offset);

/* Get the next cluster in the cluster chain, -1 if bad cluster
    or -2 if no more clusters */
int fat_get_next_cluster(void* fat_sec_buffer, uint32_t offset,
    uint8_t fat_type);




#endif /* end of include guard: _FAT_H */
