#include "initrd.h"

/*  The initrd is, essencially, a tar archive in memory.
    So this is nothing more than a tar file parser that expect
    all data is in contiguous virtual memory blocks */

static initrd_t ramdisk;

static int initrd_readdir(vfs_node_t* parent, vfs_node_t** childs);
static int initrd_read(vfs_node_t* node, uint64_t off, size_t len, void* buf);

static struct initrd_file* root;

/* Mount initrd direcly over root */
int initrd_mount() {

    struct initrd_file* file = root->child;

    vfs_node_t* vfsroot = vfs_get_root();

    if (!root) return 0;
    if (!vfsroot) return 0;

    vfsroot->flags |= VFS_FLAG_MOUNTPOINT;

    vfs_node_t *prev = NULL;

    int first = 1;
    int end = 0, child_end = 0;
    while (!end) {
        if (!file) {
            end = 1;
            break;
        }

        vfs_node_t* node = kcalloc(sizeof(vfs_node_t), 1);
        memcpy(file->name, node->name, strlen(file->name)+1);
        node->size = file->size;
        node->block = file->addr;
        node->inode = (uintptr_t)file;
        node->flags = file->flags;

        node->prev = prev;
        if (prev) {
            prev->next = node;
        }

        node->parent = vfsroot;
        node->__vfs_readdir = &initrd_readdir;
        node->__vfs_read = &initrd_read;

        prev = node;
        file = file->next;


        if (first) {
            vfsroot->ptr = node;
            first = 0;
        }
    }

    return 1;
}

static int initrd_readdir(vfs_node_t* parent, vfs_node_t** childs)
{
    /* Get the file object for this node */
    struct initrd_file* fparent = (struct initrd_file*)parent->inode;

    if (!fparent) {
        kerror("initrd: invalid inode for file %s", parent->name);
        return 0;
    }

    struct initrd_file* fchild = fparent->child;

    if (!fchild) {
        /* No childs, no need to worry */
        return 1;
    }

    vfs_node_t* prev = NULL;
    while (fchild) {

        vfs_node_t* node = kcalloc(sizeof(vfs_node_t), 1);

        if (!prev) {
            *childs = node;
        }

        memcpy(fchild->name, node->name, strlen(fchild->name)+1);
        node->size = fchild->size;
        node->block = fchild->addr;
        node->inode = (uintptr_t)fchild;
        node->flags = fchild->flags;

        node->prev = prev;
        if (prev) {
            prev->next = node;
        }

        node->parent = parent;
        node->__vfs_readdir = &initrd_readdir;
        node->__vfs_read = &initrd_read;

        prev = node;
        fchild = fchild->next;
    }
}

static int initrd_read(vfs_node_t* node, uint64_t off, size_t len, void* buf)
{
    char* ptr = (char*)(node->block & 0xffffffff);

    if (!ptr) {
        kerror("initrd: %s has an invalid pointer to data", node->name);
        return 0;
    }

    if (off > node->size) {
        kwarn("initrd: trying to read %s past its size", node->name);
        return 0;
    }

    if (len == (size_t)-1) {
        len = (size_t)node->size;
    }

    if (off+len > node->size) {
        len = (node->size-off);
    }

    memcpy(&ptr[off], buf, len);
    return len;
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

    struct initrd_file* prev = NULL;
    struct initrd_file* parent = NULL;
    struct initrd_file* child = NULL;

    /*  Parent folder name.

        tar file stores the full relative path of the file
        We get the parent name to know where the true file name starts
    */
    char parent_name[64] = "\0";
    int prev_slash_count = 0, parent_slash_count = 0;
    size_t parent_len = 0;

    while (ramdisk_ptr < ramdisk.end) {


        struct tar_block_header* blk = (struct tar_block_header*) ramdisk_ptr;
        struct initrd_file* file = kcalloc(sizeof(struct initrd_file), 1);

        if (!root) {
            root = file;
        }

        /*  If the first byte is 0, then is likely everything is 0
            So we shall end, because this is the end of the tar file data */
        if (blk->name[0] == 0) {
            break;
        }

        knotice("\traw: %s", blk->name);


        parent_len = strlen(parent_name);


        size_t size = atoi(blk->size, 8);

        /*  On tar files, file data always come after the definition,
            even on directories */
        uintptr_t addr = ramdisk_ptr + 512;

        int slash_count = 0;

        int len = strlen(blk->name);
        for (int i = 0; i < len-1; i++) {
            if (blk->name[i] == '/') {
                slash_count++;
            }
        }

        file->addr = addr;
        file->name[63] = 0;
        file->size = size;

        if (slash_count > prev_slash_count) {
            /* Previous directory was a parent directory */

            parent = prev;
            parent_slash_count = slash_count;
            prev_slash_count = slash_count+1;
            memcpy(prev->name, parent_name, strlen(prev->name)+1);
            prev = NULL;
        }

        if (slash_count < parent_slash_count) {
            /* Previous directory was the last directory of the chain */
            if (parent) {
                prev = parent;
                parent = parent->parent;
                memcpy(parent->name, parent_name, strlen(parent->name)+1);
            } else {
                prev = NULL;
                parent = NULL;
                memcpy("", parent_name, 1);
            }

            parent_slash_count = slash_count;
            prev_slash_count = slash_count-1;
        }

        prev_slash_count = slash_count;

        char* name = strrtok_n(blk->name, '/', strlen(blk->name)-2);

        if (!name)
            name = blk->name;
        else
            name = name+1;

        memcpy(name, file->name, 63);
        if (blk->typeflag == '5')
            file->flags |= VFS_FLAG_FOLDER;
        file->prev = prev;
        file->parent = parent;

        if (prev) {
            prev->next = file;
        }

        if (parent) {
            if (!parent->child) {
                parent->child = file;
            }
        }

        knotice("file: %s, parent %s (%s), prev %s, size %d",
            file->name, (parent) ? parent->name : "<NULL>", parent_name,
            (file->prev) ? file->prev->name : "<NULL>",
            file->size);

        prev = file;

        /*  Go to the next file, skip file data
            tar blocks are always aligned to 512 bytes */
        ramdisk_ptr += (512 + ((size + 511) & ~511));

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
