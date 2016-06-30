#include "vfs.h"
#include <kstdlog.h>

static vfs_node_t* root;
#define ROOT_INODE 0

/* Create the root node */
void vfs_init()
{
    root = vfs_create_node(ROOT_INODE, "/");
    root->flags = VFS_FLAG_FOLDER;
    knotice("VFS: created root node at 0x%08x", root);
}

vfs_node_t* vfs_get_root() { return root; }

vfs_node_t* vfs_create_node(uint64_t id, const char* name)
{
    vfs_node_t* n = kcalloc(sizeof(vfs_node_t),1);
    n->inode = id;
    memcpy(name, n->name, strlen(name));
    return n;
}

static vfs_filesystem_t filesystems[MAX_FS_COUNT];
unsigned fs_count = 0;
int vfs_register_fs(vfs_filesystem_t* fs)
{
    if (fs_count >= MAX_FS_COUNT)
        return 0;

    knotice("VFS: Registered filesystem %s", fs->fsname);
    filesystems[fs_count++] = *fs;
    return 1;
}

vfs_filesystem_t* vfs_get_fs(const char* name)
{
    for (unsigned i = 0; i < fs_count; i++) {
        if (!strcmp(name, filesystems[i].fsname)) {
            return &filesystems[i];
        }
    }
    return NULL;
}

vfs_filesystem_t* vfs_create_fs(const char* name)
{
    vfs_filesystem_t* fs = kcalloc(sizeof(vfs_filesystem_t), 1);
    memcpy(name, fs->fsname, strlen(name)+1);
    return fs;
}

static vfs_mount_t mounts[MAX_MOUNTS];
static unsigned mount_count = 0;

/*  Mount the device 'dev' as the filesystem 'fs' in the 'node' node.
    'node' will become the new filesystem root */
int vfs_mount(vfs_node_t* node, device_t* dev, struct vfs_filesystem* fs)
{
    if (!dev || !fs) {
        kerror("VFS: Invalid device or filesystem");
        return 0;
    }

    //knotice("%s %x %x", fs->fsname, fs->__vfs_mount, fs->__vfs_get_root_dir);
    if (!fs->__vfs_mount(dev)) {
        kerror("VFS: couldn't mount fs %s into dev %s", fs->fsname, dev->devname );
        return 0;
    }


    vfs_node_t* node_childs = NULL;
    if (!fs->__vfs_get_root_dir(dev, &node_childs)) {
        kerror("VFS: couldn't read root dir, fs %s dev %s", fs->fsname, dev->devname );
        return 0;
    }
    kprintf("||%s", dev->devname);

    node->ptr = node_childs;
    node->flags |= VFS_FLAG_MOUNTPOINT;

    vfs_mount_t m;
    m.dev = dev;
    m.fs = fs;
    m.root_dir = node;

    unsigned mountid = mount_count++;
    mounts[mountid] = m;
    while (node_childs) {
        node_childs->mount = (void*)&mounts[mountid];
        node_childs->parent = node;
        node_childs = node_childs->next;
    }

    knotice("VFS: mounted filesystem %s at device %s on %s",
        fs->fsname, dev->devname, node->name);
    return 1;
}

/*  Read the childs
        Return < 0 if error
        Return 0 if no child
        Return 1 if child
*/
int vfs_readdir(vfs_node_t* node, vfs_node_t** childs)
{
    /* If node isn't folder, return -1 */
    if (!(node->flags & VFS_FLAG_FOLDER)) return -1;

    /* If node is a mount point, return the ptr pointer */
    if (node->flags & VFS_FLAG_MOUNTPOINT) {
        /* Null? Read the root of the mountpoint which has
            this node as a rootdir parameter */
        if (!node->ptr) {

        }

        *childs = node->ptr;

        if (!*childs)
            return 0;
        else
            return 1;

    } else {
        /* If node, return the childs pointer */

        if (!node->child) {
            if (!node->__vfs_readdir(node, childs))
                return 0;

            node->child = *childs;
        } else {
            *childs = node->child;
        }

        if (!*childs)
            return 0;
        else
            return 1;

    }

    return -1;
}

/* Find a node by its path */
vfs_node_t* vfs_find_node(const char* path)
{
    // find the root
    char* path_rel = strtok(path, '/');

    if (!path_rel) {
        // how?
        kerror("ERROR: root not found! You need to use an absolute path "
        "with vfs_find_node().\n Use vfs_find_node_relative() with a relative path");
        return NULL;
    }

    path_rel++;
    return vfs_find_node_relative(vfs_get_root(), path_rel);
}

/* Find a node from a starting point */
vfs_node_t* vfs_find_node_relative(vfs_node_t* base, const char* path_rel)
{
    char *n_start, *n_end;
    n_start = path_rel;
    n_end = strtok(n_start, '/');

    vfs_node_t* node;

    int ret = vfs_readdir(base, &node);

    if (ret == 0) {
        /* No childs */
        if (!n_end) {
            /* Over. It's him. No need to search */
            return base;
        } else {
            return NULL;
        }
    } else if ( ret < 0) {
        /* Error. Return 0 */
        return NULL;
    }

    while (n_start) {
        size_t len = (n_end) ? (n_end - n_start) : strlen(n_start);
        int node_found = 0;

        while (node) {
            knotice("%s", node->name);

            if (!strncmp(node->name, n_start, len)) {
                node_found = 1;
                break;
            }
            node = node->next;
        }


        n_start = n_end;

        if (n_start) {
            n_start++;
            n_end = strtok(n_start, '/');
        }

        /* Node is found */
        if (node_found) {
            if (!n_start) {
                /* No more childs, It's him */
                return node;
            } else {
                /* Read its childs */
                vfs_node_t* daddy = node;
                vfs_readdir(daddy, &node);
            }
        }
    }

    return NULL;
}

/* Read the file content at buffer 'buf' from off to off+len.
    Returns the length read, or -1 if error.*/
int vfs_read(struct vfs_node* node, uint64_t off, size_t len,
    void* buffer)
    {
        if (node) {
            if (!(node->flags & VFS_FLAG_FOLDER)) {
                if (!node->__vfs_read) {
                    kerror("You can't read a file");
                    return -1;
                }

                return node->__vfs_read(node, off, len, buffer);
            } else {
                kerror("%s is a folder, you can't read a folder this way",
                    node->name);

                if (!node->__vfs_read) {
                    kerror("You can't read a file");
                }
            }
        }

        return -1;
    }

/* Utility conversion for unix timestamp to normal date */
void vfs_unix_to_day(int64_t timestamp,
    signed* year, unsigned* month, unsigned* day,
    unsigned* hour, unsigned* minute, unsigned* second)
    {

        *second = timestamp % 60;
        *minute = (timestamp % 3600) / 60;
        *hour = (timestamp % (3600*24)) / 3600;
        *day = (timestamp % (3600*24*30)) / (3600*24);
        *month = (timestamp % (3600*24*365)) / (3600*24*30);
        *year = 1970 + (timestamp / (3600*24*365));

        /* Correction for leap years */
        *day += ((*year-1968)/4);   //1972 is the first UNIX leap year
        if (*day > 31) {
            *day = *day - 31;
            *month = *month + 1;
        }
        if (*month > 12) {
            *month = *month - 12;
            *year = *year + 1;
        }
        /* Correction for different day-count months */

    }
