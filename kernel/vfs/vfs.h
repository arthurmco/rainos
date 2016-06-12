/*  VFS implementation for RainOS

    Copyright (C) 2016 Arthur M
*/

#ifndef _VFS_H
#define _VFS_H

struct vfs_filesystem {
    char fsname[16];
};

struct vfs_node {
    /* File name */
    char name[64];

    /* Pointer to a specific structure held by the fs driver */
    void* specific;

    /* Physical location, in fs-specific unit, of the file */
    uint64_t block;

    /* Physical file size */
    uint64_t size;

};


#endif /* end of include guard: _VFS_H */
