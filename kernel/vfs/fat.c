#include "fat.h"

static struct fat_fs fats[MAX_MOUNTS];
static unsigned fat_count = 0;

void fat_init()
{
    vfs_filesystem_t* fs = vfs_create_fs("fatfs");
    fs->__vfs_get_root_dir = &fat_get_root_dir;
    fs->__vfs_mount = &fat_mount;
    vfs_register_fs(fs);
}

int fat_mount(device_t* dev)
{
    struct fat_superblock* fat = kcalloc(512, 1);
    int r = device_read(dev, 0, 512, fat);

    if (r < 0)
        return 0;

    if (strncmp("FAT", fat->fattype.f16.fstype, 3) &&
        strncmp("FAT", fat->fattype.f32.fstype, 3)) {
            kerror("Invalid FAT superblock at %s", dev->devname);
            return 0;
    }

    unsigned char fattype = fat_get_type(fat);
    char volname[16];
    memset(volname, 0, 16);
    memcpy((fattype == 32) ? fat->fattype.f32.vol_label : fat->fattype.f16.vol_label,
        volname, 11);

    knotice("FAT: %d sectors/cluster, %d clusters, \n"
        "\tType is FAT%d\n"
        "\tVolume name: '%s'\n",
        fat->sec_clus,
        (FAT_GET_SECTORS(fat) - FAT_GET_FIRST_DATA_SECTOR(fat)) / fat->sec_clus,
        fattype, volname);

    struct fat_fs f;
    f.dev = dev;
    f.sb = fat;
    f.fat_type = fattype;
    fats[fat_count++] = f;
    return 1;
}

uint8_t fat_get_type(struct fat_superblock* sb)
{
    /*  The FAT type is determined by the cluster count
        We need, then, to get the cluster count for this disk */
    uint32_t data_sectors = FAT_GET_SECTORS(sb) - FAT_GET_FIRST_DATA_SECTOR(sb);
    uint32_t cluster_count = data_sectors / sb->sec_clus;

    if (cluster_count < 4085) {
        return 12;
    } else if (cluster_count < 65525) {
        return 16;
    } else {
        return 32;
    }
}

/* Get the cluster entry offset for a specific cluster in the FAT */
void fat_get_fat_cluster_entry(struct fat_superblock* sb, uint32_t cluster,
    /*[out]*/ uint32_t* sector, /*[out]*/ uint32_t* offset)
    {
        unsigned fat_size = FAT_GET_FAT_SIZE(sb);

        if (fat_size > 12) {
            unsigned f_offset;
            if (fat_size == 32)
                offset = cluster * 4;
            else
                offset = cluster * 2;

            *sector = sb->rsvd_secs + (f_offset / sb->bytes_sec);
            *offset = f_offset % sb->bytes_sec;

        } else {
            /* Specific bullshits for fat12 */
            kerror("fat: fat12 cluster chain traversing is unsupported");
        }
    }

/* Get the next cluster in the cluster chain, -1 if bad cluster or -2 if no more clusters */
int fat_get_next_cluster(void* fat_sec_buffer, uint32_t offset,
    uint8_t fat_type)
{
    if (fat_type > 12) {

        unsigned fat_cluster_content;

        if (fat_type == 16) {
            uint16_t* fat_sec_buf16 = (uint16_t*)fat_sec_buffer;
            fat_cluster_content = fat_sec_buf16[offset];

            if (fat_cluster_content == 0xfff7) return -1;

            return ((fat_cluster_content >= 0xfff8) ? -2 : fat_cluster_content);
        } else if (fat_type == 32) {
            uint32_t* fat_sec_buf32 = (uint32_t*)fat_sec_buffer;
            fat_cluster_content = (fat_sec_buf32[offset] & 0x0fffffff);

            if (fat_cluster_content == 0x0ffffff7) return -1;

            return ((fat_cluster_content >= 0x0ffffff8) ? -2 : fat_cluster_content);
        }
    } else {
        kerror("fat: fat12 cluster chain traversing is unsupported");
        return -1;
    }

}

static void _fat_read_directories(void* clusterbuf, unsigned int rootdir_secs,
    vfs_node_t** childs)
{
    struct fat_dir* rootdir = (struct fat_dir*)clusterbuf;
    /* 1 dir = 32 bytes, 8 dirs = 512 bytes */

    vfs_node_t* node = NULL;
    int next_inode = 1;
    for (unsigned i = 0; i < rootdir_secs*8; i++) {
        if (next_inode) {
            vfs_node_t* old_node = node;

            node = kcalloc(sizeof(vfs_node_t), 1);

            if (!old_node) {
                //node was null, this means that this is the first node
                *childs = node;

            } else {
                old_node->next = node;
            }

            node->prev = old_node;
            next_inode = 0;
        }

        char dirname[16];
        memset(dirname, 0, 16);

        if (rootdir[i].name[0] == 0xe5) /* free directory, but can be more */
            continue;

        if (rootdir[i].name[0] == 0x0) /* free directory, no more */
            break;

        if (rootdir[i].attr & FAT_ATTR_LONG_NAME) {
            kwarn("fat: long file names is not yet supported");
            continue;
        }

        memcpy(rootdir[i].name, dirname, 11);

        if (dirname[0] == 0x05)
            dirname[0] = 0xe5; //Kanji lead byte things

        /* Work with easy fields first */
        node->size = rootdir[i].size;
        node->block = (rootdir[i].cluster_high << 16) | rootdir[i].cluster_low;

        if (rootdir[i].attr & FAT_ATTR_HIDDEN)
            node->flags |= VFS_FLAG_HIDDEN;

        if (rootdir[i].attr & FAT_ATTR_DIRECTORY)
            node->flags |= VFS_FLAG_FOLDER;

        if (node->flags & VFS_FLAG_FOLDER) {
            /* Trim the spaces on folder name */
            for (int i = 11; i > 1; i--) {
                if (dirname[i] != ' ') {
                    dirname[i+1] = 0;
                    break;
                }
            }

        } else {
            unsigned int namelen = 8, extlen = 3;
            /* Retrieve name and extension, separated */

            for (int j = 0; j < 8; j++) {
                if (rootdir[i].name[j] == ' ') {
                    namelen = j;
                    break;
                }

                dirname[j] = rootdir[i].name[j];
            }

            dirname[namelen] = '.';

            for (int j = 0; j < 3; j++) {
                if (rootdir[i].name[8+j] == ' ') {
                    dirname[namelen+j] = 0;
                    extlen = j;
                    break;
                }

                dirname[namelen+j+1] = rootdir[i].name[8+j];
            }

            dirname[namelen+extlen+1] = 0;


        }
        /* And copy it */
        memcpy(dirname, node->name, strlen(dirname)+1);
        knotice("%s, cluster %d, %d bytes", dirname,
            rootdir[i].cluster_low, rootdir[i].size);


        next_inode = 1;
    }

}

int fat_get_root_dir(device_t* dev, vfs_node_t** root_childs)
{
    struct fat_fs* fat = NULL;

    for (unsigned i = 0; i < fat_count; i++) {
        if (fats[i].dev->devid == dev->devid) {
            fat = &fats[i];
        }
    }

    if (!fat) {
        kerror("fat: device %s not mounted.", dev->devname);
        return 0;
    }

    unsigned int rootdir_sec_num = FAT_GET_ROOT_SECTOR_START(fat->sb);

    if (rootdir_sec_num == 0)
        rootdir_sec_num = fat->sb->fattype.f32.root_cluster;

    unsigned int rootdir_sec_count = FAT_GET_ROOT_SECTOR_COUNT(fat->sb);

    if (rootdir_sec_count == 0)
        rootdir_sec_count = fat->sb->sec_clus;

    knotice("fat: reading root dir at sector %d, %d secs", rootdir_sec_num,
        rootdir_sec_count);

    void* rootdir_sec = kcalloc(fat->sb->bytes_sec, rootdir_sec_count);
    int r = device_read(fat->dev, rootdir_sec_num * fat->sb->bytes_sec,
        rootdir_sec_count * fat->sb->bytes_sec, rootdir_sec);
    if (r < 0) {
        kerror("fat: couldn't read root directory from device %s", dev->devname);
        return 0;
    } else if (r == 0) {
        kerror("fat: empty root directory from device %s", dev->devname);
        return 0;
    }

    knotice("Directories... ");

    _fat_read_directories(rootdir_sec, rootdir_sec_count, root_childs);

    return 1;
}


int fat_readdir(vfs_node_t* parent, vfs_node_t** childs)
{
    return 0;
}
