#include <vfs/sfs.h>

static struct sfs_fs filesystems[MAX_SFS];
static unsigned fs_count = 0;

int sfs_calculate_checksum(struct sfs_superblock* sb) {
    uint8_t* sb_bin = (uint8_t*)sb;
    uint8_t chksum = 0;

    for (size_t i = 0x01ac; i <= 0x01bd; i++) {
        chksum += sb_bin[i];
    }

    return (chksum == 0);
}

void sfs_init()
{
    vfs_filesystem_t* sfs = vfs_create_fs("simplefs");
    sfs->__vfs_mount = &sfs_mount;
    sfs->__vfs_get_root_dir = &sfs_get_root_dir;
    vfs_register_fs(sfs);
}

int sfs_mount(device_t* dev)
{
    if (fs_count >= MAX_SFS) {
        kerror("sfs: maximum mount limit reached");
        return 0;
    }

    struct sfs_superblock* sb = kcalloc(512,1);

    if (!device_read(dev, 0, 512, sb)) {
        kerror("sfs: could not read superblock");
        return 0;
    }

    if ((sb->sfs_magic_version & 0xffffff) != 0x534653) {
        kerror("sfs: wrong magic number (%x)", sb->sfs_magic_version & 0xffffff);
        return 0;
    }

    if (!sfs_calculate_checksum(sb)) {
        kerror("sfs: wrong checksum");
        return 0;
    }

    uint8_t version = (sb->sfs_magic_version & 0xff000000) >> 24;
    knotice("sfs: filesystem has version %d.%d",
        version >> 4, version & 0xf);
    knotice("sfs: %d total blocks, %d data area blocks, %d bytes in index area, "
        "%d reserved blocks",
        ((uint32_t)sb->total_block_count), ((uint32_t)sb->data_area_blocks),
        ((uint32_t)sb->index_area_bytes), ((uint32_t)sb->rsvd_block_count));
    knotice("sfs: block size is %d bytes",
        1 << (7+ sb->block_size));

    unsigned fsnum = fs_count++;
    filesystems[fsnum].dev = dev;
    filesystems[fsnum].sb = sb;

    return 1;
}

static struct sfs_file* root_file = NULL;
int sfs_get_root_dir(device_t* dev, vfs_node_t** root_childs)
{
    /*  This filesystem stores metadata in a way much similar to the tar format
        used in the ramdisk: the path is stored with the file, so, in order to
        get the root dir we'll get all directories.

        This will cost us time for the first access, but will save us a lot of
        unneeded disk reads, because the VFS will cache the directories for
        us
    */

    struct sfs_fs* fs = NULL;
    for (unsigned i = 0; i < fs_count; i++){
        if (dev->devid == filesystems[i].dev->devid) {
            fs = &filesystems[i];
            break;
        }
    }

    if (!fs) {
        kerror("sfs: device %s not mounted", dev->devname);
        return 0;
    }

    unsigned index_area_start = (fs->sb->total_block_count -
            (1+(fs->sb->index_area_bytes) / (1 << (7 + fs->sb->block_size))));
    knotice("sfs: index area starts at block %d", index_area_start);

    unsigned index_area_off = (index_area_start) * (1 << (7 + fs->sb->block_size));
    unsigned index_area_len = (fs->sb->total_block_count - index_area_start) * (1 << (7 + fs->sb->block_size));
    //index_area_len--;

    void* index_area = kcalloc(index_area_len, 1);
    fs->index_area = index_area;

    if (!device_read(dev, index_area_off, index_area_len, index_area)) {
        kerror("sfs: could not read index area from %s", dev->devname);
        return 0;
    }
    knotice("sfs: index area loaded at 0x%08x", index_area);

    sfs_parse_index_area(fs);

    struct sfs_file* file = root_file;
    vfs_node_t *first=NULL, *prev=NULL;
    while (file) {
        vfs_node_t* node = kcalloc(sizeof(vfs_node_t),1);
        memcpy(file->name, node->name, 63);
        if (file->isDir) node->flags |= VFS_FLAG_FOLDER;
        node->size = file->file_size;
        node->block = file->bstart;
        node->inode = (uint64_t)file;
        node->date_creation = file->timestamp;
        node->date_modification = file->timestamp;
        node->__vfs_readdir = &sfs_readdir;
        node->__vfs_read = &sfs_read;

        if (first){
            prev->next = node;
            node->prev = prev;
        } else {
            first = node;
        }
        prev = node;

        file = file->next;
    }

    *root_childs = first;
    return 1;
}



/*  Parse index area, returns a sfs_file object representing the first
    item of root directory */
struct sfs_file* sfs_parse_index_area(struct sfs_fs* fs)
{
    unsigned index_blkcount = 1+(fs->sb->index_area_bytes / (1 << (7 + fs->sb->block_size)));
    unsigned index_count = (fs->sb->index_area_bytes / sizeof(struct sfs_index));

    knotice("sfs: we have %d items, occupying %d blocks",
        index_count, index_blkcount);

    struct sfs_file *parent = NULL, *prev = NULL;
    struct sfs_index* indexarea = (struct sfs_index*)fs->index_area;
    /* Read the index area */
    for (int i = (index_blkcount * (512/sizeof(struct sfs_index)))-1; i >= 0; i--) {
        knotice("entry type %02x", indexarea[i].entry_type);
        switch (indexarea[i].entry_type) {
        case SFS_ENTRY_VOLUMEID:
            {
            size_t vlen = strlen(indexarea[i].entry_data.volumeid.volume_name)+1;
            char* vol = kmalloc(vlen);
            memcpy(indexarea[i].entry_data.volumeid.volume_name, vol, vlen);
            fs->volname = vol;
            knotice("sfs: volume name is %s", vol);
        } break;

        case SFS_ENTRY_FILE:
        {
            struct sfs_file* file = kcalloc(sizeof(struct sfs_file),1);

            /* Check if parent is still parent */
            if (parent) {
                if (!strncmp(indexarea[i].entry_data.fileentry.dirname,
                            indexarea[parent->index].entry_data.direntry.dirname,
                            strlen(indexarea[parent->index].entry_data.direntry.dirname)-1)) {
                    knotice("childs of %s", parent->name);

                    if (!parent->child) {
                        parent->child = file;
                    }
                } else {
                    /* No parent anymore */
                    prev = parent;
                    parent = NULL;
                }
            }

            if (!root_file) {
                root_file = file;
                prev = file;
            } else {
                if (prev)
                    prev->next = file;
                file->prev = prev;
            }

            /* Hide the dir name, plus the slash */
            size_t pName = (parent) ? strlen(parent->name)+1 : 0;

            file->name = &(indexarea[i].entry_data.fileentry.dirname[pName]);
            file->file_size = indexarea[i].entry_data.fileentry.file_size;
            file->bstart = indexarea[i].entry_data.fileentry.starting_block;
            file->bend = indexarea[i].entry_data.fileentry.ending_block;
            file->fs = fs;
            file->index = i;
            /* Convert it to the RainOS format, ie the normal unix time */
            file->timestamp = (indexarea[i].entry_data.fileentry.timestamp/65536);

            knotice("%d: %s size %d blocks %d-%d timestamp %u",
                i, file->name, (uint32_t)file->file_size,
                (uint32_t)file->bstart, (uint32_t)file->bend,
                (uint32_t)file->timestamp);

            file->parent = parent;
            prev = file;
        }
            break;

        case SFS_ENTRY_DIRECTORY:
        {
        struct sfs_file* file = kcalloc(sizeof(struct sfs_file),1);

        /* Check if parent is still parent */
        if (parent) {
            if (!strncmp(indexarea[i].entry_data.fileentry.dirname,
                        parent->name, strlen(parent->name)-1)) {
                knotice("childs of %s",
                indexarea[i].entry_data.fileentry.dirname);
            } else {
                /* No parent anymore */
                prev = parent;
                parent = NULL;
            }
        }

        if (!root_file) {
            root_file = file;
            prev = file;
        } else {
            prev->next = file;
            file->prev = prev;
        }


        size_t pName = parent ? strlen(parent->name)+1 : 0;

        /* Hide the dir name, plus the slash */
        file->name = &indexarea[i].entry_data.direntry.dirname[pName];
        file->timestamp = indexarea[i].entry_data.direntry.timestamp;
        file->isDir = 1;
        file->index = i;
        file->fs = fs;
        knotice("%d: %s dir timestamp %u",
            i, file->name, (uint32_t)file->file_size,
            (uint32_t)file->bstart, (uint32_t)file->bend,
            (uint32_t)file->timestamp);

        parent = file;
        prev = NULL;

        } break;
        case SFS_ENTRY_STARTMARKER:
            knotice("starting marker here, ending...");
            goto end_loop;
            break;
        }
    }

    end_loop:
    return root_file;
}

int sfs_readdir(vfs_node_t* node, vfs_node_t** childs)
{
    struct sfs_file* fdir = (struct sfs_file*)(node->inode & 0xffffffff);
    struct sfs_file* file = fdir->child;

    *childs = NULL;
    vfs_node_t* prev = NULL;

    while (file) {
        vfs_node_t* node = kcalloc(sizeof(vfs_node_t),1);
        if (!*childs)   *childs = node;

        memcpy(file->name, node->name, 63);
        if (file->isDir) node->flags |= VFS_FLAG_FOLDER;
        node->size = file->file_size;
        node->block = file->bstart;
        node->inode = (uint64_t)file;
        node->date_creation = file->timestamp;
        node->date_modification = file->timestamp;
        node->__vfs_readdir = &sfs_readdir;
        node->__vfs_read = &sfs_read;

        if (prev)
            prev->next = node;

        node->prev = prev;
        prev = node;

        file = file->next;
    }

    return 1;
}

int sfs_read(vfs_node_t* node, uint64_t off, size_t len, void* buffer)
{
    struct sfs_file* file = (struct sfs_file*)(node->inode & 0xffffffff);
    if (!file) {
        kerror("sfs: invalid file");
        return 0;
    }

    int blk_size = 1 << (7+file->fs->sb->block_size);
    int data_area_start = (file->fs->sb->rsvd_block_count * blk_size);

    if (!buffer) {
        kerror("sfs: (%s) buffer is null", node->name);
        return 0;
    }

    int block_offset = (file->bstart-1)*blk_size;
    knotice("sfs: reading %s, block %d (data area off %x, disk off %x) "
        ", %u bytes",
        file->name, (uint32_t)(file->bstart), block_offset,
        block_offset+data_area_start, len);

    if (!device_read(file->fs->dev, (uint64_t)(data_area_start+block_offset),
        len, buffer)) {
        kerror("sfs: device error while reading %s", node->name);
        return 0;
    }

    return 1;
}
