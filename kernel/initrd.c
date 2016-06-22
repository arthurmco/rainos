#include "initrd.h"

/*  The initrd is, essencially, a tar archive in memory.
    So this is nothing more than a tar file parser that expect
    all data is in contiguous virtual memory blocks */

static initrd_t ramdisk;

static int initrd_readdir(vfs_node_t* parent, vfs_node_t** childs);
static int initrd_read(vfs_node_t* node, uint64_t off, size_t len, void* buf);

/* Mount initrd direcly over root */
int initrd_mount() {

}

/*  Initialize the initrd
    Returns 1 if succeded, 0 if not */
int initrd_init(uintptr_t start, uintptr_t end)
{
    ramdisk.start = start;
    ramdisk.end = end;

    knotice("INITRD: Detected 0x%08x-0x%08x area as the initrd",
            start, end);

    initrd_register();
    if (!initrd_verify_checksum((struct tar_block_header*)ramdisk.start)){
        kerror("INITRD: checksum failed");
        return 0;
    }

    /* Organize the tar files in a hierarchical structure */
    uintptr_t ramdisk_ptr = ramdisk.start;

    struct initrd_file* next = NULL;
    struct initrd_file* prev = NULL;
    struct initrd_file* parent = NULL;
    struct initrd_file* child = NULL;

    while (ramdisk_ptr < ramdisk.end) {
        struct tar_block_header* blk = (struct tar_block_header*) ramdisk_ptr;
        struct initrd_file* file = kcalloc(sizeof(struct initrd_file), 1);

        /*  If the first byte is 0, then is likely everything is 0
            So we shall end, because this is the end of the tar file data */
        if (blk->name[0] == 0) {
            break;
        }

        char fname[128];

        /*  On tar files, file data always come after the definition,
            even on directories */
        uintptr_t addr = ramdisk_ptr + 512;

        size_t size = 0;

        memset(fname, 0, 128);
        memcpy(blk->name, fname, 100);

        char strsize[16];
        memset(strsize, 0, 16);
        memcpy(blk->size, strsize, 12);
        size = atoi(strsize, 8);

        knotice("\t %08x: %s, %d bytes ", ramdisk_ptr, fname, size);

        /*  Go to the next file, skip file data
            tar blocks are always aligned to 512 bytes */
        ramdisk_ptr += (512 + ((size + 511) & ~511));

        memcpy(&fname[0], file->name, strlen(file->name)+1);
        file->size = size;
        file->addr = addr;

        file->prev = prev;
        if (prev)
            prev->next = file;

        prev = file;
    }

    return 1;
}

/* Verify the initrd checksum
    1 if ok, 0 if not
*/
int initrd_verify_checksum(struct tar_block_header* hdr)
{
    uint8_t* field = (uint8_t*)hdr;
    uint32_t checksum = 0;

    for (unsigned off = 0; off < 500; off++) {
        if (off >= 148 && off < 156)
            /* Calculate checksum field as if it is zero padded */
            checksum += ' ';
        else
            checksum += field[off];
    }

    char file_checksum_str[12];
    memset(file_checksum_str, 0, 12);
    memcpy(hdr->chksum, file_checksum_str, 8);
    uint32_t file_checksum = atoi(file_checksum_str, 8);

    knotice("checksum: %d = %d", checksum, file_checksum);

    return (file_checksum == checksum);
}

/* Register initrd filesystem in the vfs */
void initrd_register()
{
    vfs_filesystem_t* initrdfs = vfs_create_fs("initrdfs");

    vfs_register_fs(initrdfs);
}

static int initrd_readdir(vfs_node_t* parent, vfs_node_t** childs);
static int initrd_read(vfs_node_t* node, uint64_t off, size_t len, void* buf);
