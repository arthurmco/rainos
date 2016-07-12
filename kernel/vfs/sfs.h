/*  SimpleFS file system driver
    (from http://wiki.osdev.org/SFS)

    Copyright (C) 2016 Arthur M
*/

#include "vfs.h"

#ifndef _SFS_H
#define _SFS_H

struct sfs_superblock {
    char bootcode[11];
    char bpb[21];
    char bootcode2[372];
    uint64_t timestamp;
    uint64_t data_area_blocks;
    uint64_t index_area_blocks;
    uint32_t sfs_magic_version;
    uint64_t total_block_count;
    uint32_t rsvd_block_count;
    uint8_t block_size; /* Usually 2 for 512byte blocks */
    uint8_t checksum;
} __attribute__((packed));

#define MAX_SFS 8
struct sfs_fs {
    device_t* dev;
    struct sfs_superblock* sb;
    void* index_area;
};

struct sfs_volumeid {
    char unused[3];
    uint64_t format_timestamp;
    char volume_name[52];
} __attribute__((packed));

struct sfs_direntry {
    uint8_t cont_entry_count;
    uint64_t timestamp;
    char dirname[54];
} __attribute__((packed));

struct sfs_fileentry {
    uint8_t cont_entry_count;
    uint64_t timestamp;
    uint64_t starting_block;
    uint64_t ending_block;
    uint64_t file_size; /* in bytes */
    char dirname[30];
} __attribute__((packed));

struct sfs_index {
    uint8_t entry_type;
    union sfs_entries {
        struct sfs_volumeid volumeid;
        struct sfs_direntry direntry;
        struct sfs_fileentry fileentry;
    } entry_data;
} __attribute__((packed));

enum SFSEntryType {
    SFS_ENTRY_VOLUMEID = 0x01,
    SFS_ENTRY_STARTMARKER = 0x02,
    SFS_ENTRY_UNUSED = 0x10,
    SFS_ENTRY_DIRECTORY = 0x11,
    SFS_ENTRY_FILE = 0x12,
    SFS_ENTRY_UNUSABLE = 0x18,
    SFS_ENTRY_DELETEDDIR = 0x19,
    SFS_ENTRY_DELETEDFILE = 0x1A,
};

void sfs_init();

int sfs_mount(device_t*);
int sfs_get_root_dir(device_t* dev, vfs_node_t** root_childs);

/* Parse index area, returns root directories */
int sfs_parse_index_area(struct sfs_fs* fs, vfs_node_t** root);


#endif /* end of include guard: _SFS_H */
