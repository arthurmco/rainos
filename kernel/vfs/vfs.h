/*  VFS implementation for RainOS

    Copyright (C) 2016 Arthur M
*/

#ifndef _VFS_H
#define _VFS_H

#include <kstring.h>

#include "../dev.h"

typedef struct vfs_filesystem {
    /* Filesystem name */
    char fsname[16];

    int (*__vfs_mount)(device_t* dev);
} vfs_filesystem_t ;

enum VfsNodeFlags {
    VFS_FLAG_HIDDEN = 0x01,     /* Node is hidden */

    /* Node is a link. In this case, 'ptr' is where the link points to */
    VFS_FLAG_SYMLINK = 0x02,

    /* Node is a folder */
    VFS_FLAG_FOLDER = 0x04,

    /*  Node is a mountpoint. In this case, 'ptr' is the root dir of
        the disk you mounted */
    VFS_FLAG_MOUNTPOINT = 0x08,

};

typedef struct vfs_node {
    /* File name */
    char name[64];

    /* Node options */
    uint32_t flags;

    /* Inode number, used as a ID to the fs driver */
    uint64_t inode;

    /* Physical location, in fs-specific unit, of the file */
    uint64_t block;

    /* Physical file size */
    uint64_t size;

    /* A pointer to the mount point.
        Held as void to avoid circular reference errors */
    void* mount;

    /* Read a directory and return its childs. */
    int (*__vfs_readdir)(struct vfs_node* node, struct vfs_node** child);

    /*  Read a directory content. It's recommended that 'node' should be
        a file .
            'off' is the byte offset.
            'len' is the length of the read, or -1 to read all file
            'buf' is the buffer.
        */
    int (*__vfs_read)(struct vfs_node* node, uint64_t off, size_t len,
        void* buffer);

    /* Write a directory */
    int (*__vfs_writedir)(struct vfs_node* node, struct vfs_node* child);

    /*  Write a file. This will overwrite the file data between 'off' and
        'off+len'. If you want to append, use off = -1 */
    int (*__vfs_write)(struct vfs_node* node, uint64_t off, size_t len,
        void* buffer);


    /* Pointer to the first child */
    struct vfs_node* child;
    struct vfs_node* ptr;
    struct vfs_node* parent;

    // I honestly hate this pointer syntax, but it's prettier on this case
    struct vfs_node *next, *prev;
} vfs_node_t;


/*  A mount point.
    Essentially, a filesystem+device combination

    Remember that the device must support block functions (read() and write()) */
typedef struct vfs_mount {
    struct vfs_filesystem* fs;
    device_t* dev;
    vfs_node_t* root_dir;

    int (*__vfs_get_root_dir)(struct vfs_mount*);
} vfs_mount_t;

/* Create the root node */
void vfs_init();
vfs_node_t* vfs_get_root();

vfs_node_t* vfs_create_node(uint64_t id, const char* name);

int vfs_register_fs(vfs_filesystem_t* fs);
vfs_filesystem_t* vfs_get_fs(const char* name);

/*  Mount the device 'dev' as the filesystem 'fs' in the 'node' node.
    'node' will become the new filesystem root */
int vfs_mount(vfs_node_t* node, device_t* dev, struct vfs_filesystem* fs);

/*  Read the childs and store the first child on
    node->childs member */
int vfs_readdir(vfs_node_t* node, vfs_node_t** childs);

/* Find a node by its path */
vfs_node_t* vfs_find_node(const char* path);

/* Find a node from a starting point */
vfs_node_t* vfs_find_node_relative(vfs_node_t* base, const char* path_rel);

#endif /* end of include guard: _VFS_H */
