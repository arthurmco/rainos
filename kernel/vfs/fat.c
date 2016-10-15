#include "fat.h"

static struct fat_fs fats[MAX_MOUNTS];
static unsigned fat_count = 0;

static int fat_readdir(vfs_node_t* parent, vfs_node_t** childs);
static int fat_read(vfs_node_t* node, uint64_t off, size_t len, void* buf);

/* Read the directories
    Return 1 if you need to read one more cluster to it
        clusterbuf: Buffer where the dirs are
        dir_sec_count: How much sectors 'clusterbuf' have
        childs: Where do you want to store the childs
        last_sibling: The last sibling, for continue to read from previous
            reads */
static int _fat_read_directories(void* clusterbuf, unsigned int dir_sec_count,
    vfs_node_t** childs, vfs_node_t* parent, vfs_node_t* last_sibling);

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

    if (strncmp("FAT\0", fat->fattype.f16.fstype, 3) &&
        strncmp("FAT\0", fat->fattype.f32.fstype, 3)) {
            kerror("Invalid FAT superblock at %s ", dev->devname);
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
                f_offset = cluster * 4;
            else
                f_offset = cluster * 2;

            *sector = sb->rsvd_secs + (f_offset / sb->bytes_sec);
            *offset = f_offset % sb->bytes_sec;

        } else {
            /* Specific bullshits for fat12 */
            kwarn("fat: fat12 cluster chain traversing is experimental");
            unsigned f_offset = cluster + (cluster / 2);
            *sector = sb->rsvd_secs + (f_offset / sb->bytes_sec);
            *offset = f_offset % sb->bytes_sec;

            /*  Trick to store what we need to do with fat12 cluster, since
                a cluster can be in a middle of a byte. */
            if (cluster & 1)
                *offset |= 0x80000000;

        }
    }

/* Get the next cluster in the cluster chain, -1 if bad cluster or -2 if no more clusters */
int fat_get_next_cluster(char* fat_sec_buffer, uint32_t offset,
    uint8_t fat_type)
{
    if (fat_type > 12) {

        int fat_cluster_content;

        if (fat_type == 16) {
            fat_cluster_content = *(uint16_t*)&fat_sec_buffer[offset];

            if (fat_cluster_content == 0xfff7) return -1;

            return ((fat_cluster_content >= 0xfff8) ? -2 : fat_cluster_content);
        } else if (fat_type == 32) {
            fat_cluster_content = ((*(uint32_t*)&fat_sec_buffer[offset]) & 0x0fffffff);

            if (fat_cluster_content == 0x0ffffff7) return -1;

            return ((fat_cluster_content >= 0x0ffffff8) ? -2 : fat_cluster_content);
        }
    } else {
        kwarn("fat: fat12 cluster chain traversing is experimental");
        uint16_t fat_cluster_content;

        fat_cluster_content = *(uint16_t*)&fat_sec_buffer[offset & 0x7fffffff];
        uint16_t fat_cluster_next;

        if (offset & 0x80000000)
            fat_cluster_next = (fat_cluster_content >> 4);
        else
            fat_cluster_next = (fat_cluster_content & 0xfff);

        if (fat_cluster_content == 0x0ff7) return -1;

        if (fat_cluster_content >= 0x0ff8)
            return -2;
        else
            return fat_cluster_content;

    }

    return -1;
}


/* Read the directories
    Return 1 if you need to read one more cluster to it
        clusterbuf: Buffer where the dirs are
        dir_sec_count: How much sectors 'clusterbuf' have
        childs: Where do you want to store the childs
        last_sibling: The last sibling, for continue to read from previous
            reads */
static int _fat_read_directories(void* clusterbuf, unsigned int dir_sec_count,
    vfs_node_t** childs, vfs_node_t* parent, vfs_node_t* last_sibling)
{
    vfs_node_t* node = last_sibling;

    if (dir_sec_count == 0) {
        /* TODO: Read clusters on demand */
        dir_sec_count = 2;
    }

    struct fat_dir* rootdir = (struct fat_dir*)clusterbuf;
    /* 1 dir = 32 bytes, 8 dirs = 512 bytes */

    int next_inode = 1;
    int is_over = 0;

    char long_name[128];
    int long_off = 0;
    int has_long_name = 0;

    for (unsigned i = 0; i < dir_sec_count*8; i++) {
        if (((unsigned char)rootdir[i].name[0]) == 0xe5) {/* free directory, but can be more */
            long_off = 0;
            has_long_name = 0;
            continue;
        }

        /* Illegal chars on a directory entry */
        if (((unsigned char)rootdir[i].name[0]) < 0x20 &&
            rootdir[i].name[0] != 0x05 &&
            rootdir[i].attr != FAT_ATTR_LONG_NAME ) {
                continue;
            }

        if (rootdir[i].name[0] >= 0x2a &&
            rootdir[i].name[0] <= 0x2f) {
                continue;
            }

        if (rootdir[i].name[0] == 0x0) { /* free directory, no more */
            is_over = 1;
            break;
        }

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

        if (rootdir[i].attr == FAT_ATTR_LONG_NAME) {
            /* Get the long name for the file */
            struct fat_long_dir* long_dir = (struct fat_long_dir*) &rootdir[i];
            if (long_dir->type != 0) {
                kwarn("fat: entry %d is a invalid long name entry", i);
                long_off = 0;
                continue;
            }

            long_off = ((long_dir->ordinal & 0x1f)-1) * 13;
            knotice("%d", long_off);

            for (int c = 0; c < 5; c++)
                long_name[long_off+c] = (char)(long_dir->name1[c] & 0xff);

            for (int c = 0; c < 6; c++)
                long_name[long_off+5+c] = (char)(long_dir->name2[c] & 0xff);

            for (int c = 0; c < 2; c++)
                long_name[long_off+11+c] = (char)(long_dir->name3[c] & 0xff);


            if (long_dir->ordinal & 0x40) {
                /* The last. Zero-pad the name */
                long_name[long_off+13] = 0;
            }

            has_long_name = 1;
            continue;
        }

        memcpy(rootdir[i].name, dirname, 11);

        if (dirname[0] == 0x05)
            dirname[0] = 0xe5; //Kanji lead byte things

        /* Work with easy fields first */
        node->size = rootdir[i].size;
        node->block = ((uint32_t)rootdir[i].cluster_high << 16) | (uint32_t)rootdir[i].cluster_low;

        if (rootdir[i].attr & FAT_ATTR_HIDDEN)
            node->flags |= VFS_FLAG_HIDDEN;

        if (rootdir[i].attr & FAT_ATTR_DIRECTORY)
            node->flags |= VFS_FLAG_FOLDER;

        if (has_long_name) {
            /* It was a long name entry. Copy it */
            memcpy(long_name, node->name, strlen(long_name));
            long_off = 0;
            has_long_name = 0;
            long_name[0] = '1';
        } else {
            if (node->flags & VFS_FLAG_FOLDER) {
            /* Trim the spaces on folder name */
            for (int i = 10; i > 1; i--) {
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
                        dirname[namelen+j+1] = 0;
                        extlen = j;
                        break;
                    }

                    dirname[namelen+j+1] = rootdir[i].name[8+j];
                }

                dirname[namelen+extlen+1] = 0;
            }
        /* And copy it */

        memcpy(dirname, node->name, strlen(dirname)+1);
        }
        knotice("%s %d", node->name, rootdir[i].cluster_low);

        node->__vfs_readdir = &fat_readdir;
        node->__vfs_read = &fat_read;
        node->parent = parent;

        if (parent) {
            node->mount = parent->mount;
        }
        next_inode = 1;
    }

    /* Check if we need to read another cluster */
    if (!is_over) {
        return 1;
    }

    return 0;

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

    _fat_read_directories(rootdir_sec, rootdir_sec_count,
        root_childs, NULL, NULL);

    kfree(rootdir_sec);

    return 1;
}


int fat_readdir(vfs_node_t* parent, vfs_node_t** childs)
{
    uint32_t clus = (uint32_t)parent->block & 0xffffffff;
    uint32_t sec;

    device_t* d = ((vfs_mount_t*)parent->mount)->dev;
    struct fat_fs* fs = NULL;

    FAT_GET_DEVICE(fs, d);

    if (!fs) {
        kerror("%s node wasn't mounted as a FAT filesystem", parent->name);
        return 0;
    }

    /* Get cluster number and offset */
    sec = FAT_GET_FIRST_SECTOR_CLUSTER(fs->sb, clus);

    void* clus_buf = kcalloc(fs->sb->bytes_sec, fs->sb->sec_clus);
    // knotice("%d %d %d", clus, sec, fs->sb->sec_clus*fs->sb->bytes_sec);
    // knotice("%x %s ~~~", d, d->devname);

    int r = device_read(d, sec * fs->sb->bytes_sec, fs->sb->sec_clus * fs->sb->bytes_sec,
        clus_buf);

    if (r < 0) {
        kerror("fat: couldn't read dir %s directory from device %s",
            parent->name, d->devname);
        return 0;
    } else if (r == 0) {
        kerror("fat: empty directory %s from device %s",
            parent->name, d->devname);
        return 0;
    }


    vfs_node_t* last_child = NULL;

    int not_over = 1;
    do {
        not_over = _fat_read_directories(clus_buf, fs->sb->sec_clus, childs,
            parent, last_child);

        if (!not_over) break;

        if (!last_child) {
            last_child = *childs;

        }
        // knotice("fat: more one cluster");

        if (not_over) {
            uint32_t next_clus_sec, next_clus_off;
            fat_get_fat_cluster_entry(fs->sb, clus, &next_clus_sec, &next_clus_off);

            knotice(">>> clus %d, nextsec: %d, nextoff: %d (%x)", clus, next_clus_sec, next_clus_off, next_clus_off);
            /* Don't mind reusing buffers */
            r = device_read(d, next_clus_sec * fs->sb->bytes_sec, fs->sb->bytes_sec,
                clus_buf);

            if (r < 0) {
                kerror("fat: couldn't read dir %s directory from device %s",
                    parent->name, d->devname);
                return 0;
            } else if (r == 0) {
                kerror("fat: empty directory %s from device %s",
                    parent->name, d->devname);
                return 0;
            }

            r = fat_get_next_cluster(clus_buf, next_clus_off, fs->fat_type);
            // knotice("nextclus: %d", r);

            switch (r) {
                case -1:
                    /* Bad cluster */
                    kwarn("fat: bad cluster at cluster pointed from %d", clus);
                    return 0;

                case 0:
                case -2:
                    /* End of all */
                    not_over = 0;
                    goto read_end;
                    break;

                default:
                    clus = r;
                    knotice("next cluster is %d", clus);
                    break;
            }

            sec = FAT_GET_FIRST_SECTOR_CLUSTER(fs->sb, clus);

            r = device_read(d, sec * fs->sb->bytes_sec, fs->sb->sec_clus * fs->sb->bytes_sec,
                clus_buf);

            knotice("read at sec %d", sec);


            while (!last_child || !last_child->next) {
                last_child = last_child->next;
            }

        }

    } while (not_over);

read_end:
//    knotice("é 13 porha");
    kfree(clus_buf);
//    knotice("BIRR");
    return 1;
}

/* Read a file. */
static int fat_read(vfs_node_t* node, uint64_t off, size_t len, void* buf)
{
    uint32_t clus = (uint32_t)node->block & 0xffffffff;
    uint32_t sec;

    device_t* d = ((vfs_mount_t*)node->mount)->dev;
    struct fat_fs* fs = NULL;
    FAT_GET_DEVICE(fs, d);

    if (!fs) {
        kerror("%s node wasn't mounted as a FAT filesystem", node->name);
        return 0;
    }

    /* Get cluster number and offset */
    sec = FAT_GET_FIRST_SECTOR_CLUSTER(fs->sb, clus);

    char* clus_buf = kcalloc(fs->sb->bytes_sec, fs->sb->sec_clus);

    int r = device_read(d, sec * fs->sb->bytes_sec, fs->sb->sec_clus * fs->sb->bytes_sec,
        clus_buf);

    if (r < 0) {
        kerror("fat: couldn't read file %s from device %s",
            node->name, d->devname);
        return 0;

    } else if (r == 0) {
        kerror("fat: empty file %s from device %s",
            node->name, d->devname);
        return 0;
    }

    if (off > node->size) {
        kwarn("fat: trying to read %s past its length", node->name);
        return 0;
    }

    if (len == (unsigned int)-1)
        len = node->size;

    unsigned int limit = (uint32_t)(node->size > (off+len)) ? (off+len) : node->size;
    unsigned int len_read = 0;
    const unsigned int bytes_per_clus = fs->sb->bytes_sec * fs->sb->sec_clus;
    unsigned int clus_off = (uint32_t)off;

    do {

        unsigned int data_read = (bytes_per_clus > (limit-off)) ? (limit - off)
            : bytes_per_clus;


        /* Check if we've gone through a cluster boundary */
        if ((clus_off+len) > bytes_per_clus) {
            /* Align it */
            data_read = bytes_per_clus - clus_off;
        }

        /* If there's data in this cluster */
        if (data_read > 0) {
            char* c_buf = (char*)buf;
            memcpy(&clus_buf[clus_off], &c_buf[len_read], data_read);
        }

        len_read += data_read;
        off += len_read;

        if (off > limit) {

            /* already ended, no need to access disk */
            break;
        }

        /* Tries to read the next cluster */
        uint32_t next_clus_sec, next_clus_off;
        fat_get_fat_cluster_entry(fs->sb, clus, &next_clus_sec, &next_clus_off);

        /* Don't mind reusing buffers */
        r = device_read(d, next_clus_sec * fs->sb->bytes_sec, fs->sb->bytes_sec,
            clus_buf);

        if (r <= 0) {
            kerror("fat: couldn't read cluster entry on FAT for clus %d", clus);
            return 0;
        }

        r = fat_get_next_cluster(clus_buf, next_clus_off, fs->fat_type);
        knotice("fat: next cluster is %d", r);

        switch (r) {
            case -1:
                /* Bad cluster */
                kwarn("fat: bad cluster at cluster pointed from %d", clus);
                return 0;

            case 0:
            case -2:
                /* End of all */
                len_read = (limit);
                break;

            default:
                clus = r;
                knotice("next cluster is %d", clus);
                break;
        }

        sec = FAT_GET_FIRST_SECTOR_CLUSTER(fs->sb, clus);

        r = device_read(d, sec * fs->sb->bytes_sec, bytes_per_clus, clus_buf);
        if (r <= 0) {
            kerror("fat: error while reading cluster %d", clus);
            return r;
        }

        clus_off = 0;

    } while (len_read < (len-off));

    kfree(clus_buf);

    return len_read;
}
