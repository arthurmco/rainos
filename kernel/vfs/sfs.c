#include "sfs.h"

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
    knotice("sfs: %d total blocks, %d data area blocks, %d index area blocks, "
        "%d reserved blocks",
        ((uint32_t)sb->total_block_count), ((uint32_t)sb->data_area_blocks),
        ((uint32_t)sb->index_area_blocks), ((uint32_t)sb->rsvd_block_count));
    knotice("sfs: block size is %d bytes",
        1 << (7+ sb->block_size));

    unsigned fsnum = fs_count++;
    filesystems[fsnum].dev = dev;
    filesystems[fsnum].sb = sb;

    return 1;
}

int sfs_get_root_dir(device_t* dev, vfs_node_t** root_childs)
{
    /*  This filesystem stores metadata in a way much similar to the tar format
        used in the ramdisk: the path is stored with the file, so, in order to
        get the root dir we'll get all directories.

        This will cost us time for the first access, but will save us a lot of
        unneeded disk reads
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

    unsigned index_area_start = (fs->sb->total_block_count - fs->sb->index_area_blocks);
    knotice("sfs: index area starts at block %d", index_area_start);

    unsigned index_area_off = index_area_start * (1 << (7 + fs->sb->block_size));
    unsigned index_area_len = (fs->sb->index_area_blocks * (1 << (7 + fs->sb->block_size)));

    void* index_area = kcalloc(index_area_len, 1);
    fs->index_area = index_area;

    if (!device_read(dev, index_area_off, index_area_len, index_area)) {
        kerror("sfs: could not read index area from %s", dev->devname);
        return 0;
    }

    sfs_parse_index_area(fs, root_childs);

    return 0;
}

int sfs_parse_index_area(struct sfs_fs* fs, vfs_node_t** root)
{
    unsigned index_count = (fs->sb->index_area_blocks * (1 << (7 + fs->sb->block_size)));
    index_count /= sizeof(struct sfs_index);

    knotice("sfs: %d total index entries (%d entry size)", index_count,
        sizeof(struct sfs_index));
    struct sfs_index* indices = (struct sfs_index*) fs->index_area;

    for (size_t i = 0; i < index_count; i++) {
        if (!indices[i].entry_type) continue;
        knotice("\t entry type %02x", indices[i].entry_type);
    }

    return 1;

}
