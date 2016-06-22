#include "initrd.h"

/*  The initrd is, essencially, a tar archive in memory.
    So this is nothing more than a tar file parser that expect
    all data is in contiguous virtual memory blocks */

static struct initrd ramdisk;

/*  Initialize the initrd
    Returns 1 if succeded, 0 if not */
int initrd_init(uintptr_t start, uintptr_t end)
{
    ramdisk.start = start;
    ramdisk.end = end;

    knotice("INITRD: Detected %08x-%08x area as the initrd",
            start, end);

    initrd_register();

    return 0;
}

/* Verify the initrd checksum
    1 if ok, 0 if not
*/
int initrd_verify_checksum(struct initrd* rd)
{
    return 0;
}

/* Register initrd filesystem in the vfs */
void initrd_register()
{
    vfs_filesystem_t* initrdfs = vfs_create_fs("initrdfs");

    vfs_register_fs(initrdfs);
}
