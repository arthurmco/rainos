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

    /* Check if isn't mounted already */
    for (size_t i = 0; i < mount_count; i++) {
        if (mounts[i].fs == fs && mounts[i].dev == dev &&
            mounts[i].root_dir == node) {
                kerror("VFS: this filesystem is already mounted here.");
                return 0;
            }
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

    device_ioctl(dev, IOCTL_SET_MOUNTED, NULL, 1, NULL);
    knotice("VFS: mounted filesystem %s at device %s on %s",
        fs->fsname, dev->devname, node->name);
    return 1;
}

/* Free the sons recursively */
int vfs_free_sons(vfs_node_t* daddy, int sons_are_mounted) {
    if (!daddy) return 1; //no children

    vfs_node_t* first_son = (sons_are_mounted) ? daddy->ptr : daddy->child;

    if (first_son) {
        vfs_node_t* bros = first_son;
        vfs_node_t* next;
        while (bros) {
            char p[128];
            vfs_get_full_path(bros, p);
            knotice("VFS: Freeing %s", p);

            next = bros->next;

            if (!vfs_free_sons(bros->child, 0)) {

                vfs_get_full_path(bros->child, p);
                kerror("VFS: failed to free vfs node at %s\n"
                    "You should restart now.", p);
                return 0;
            }
            bros->prev = NULL;

            kfree(bros);
            bros = next;
        }
    }

    /* avoid dangling pointers */
    if (sons_are_mounted) daddy->ptr = NULL;
    else daddy->child = NULL;

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

    /* Check if the media changed */
    vfs_mount_t* m = (vfs_mount_t*)node->mount;
    if (m) {
        uint32_t ret = 0;
        if (device_ioctl(m->dev, IOCTL_CHANGED, &ret, 0, 0)) {
            if (ret == 1 || ret == 0xffffffff) {
                /* Free the node pointers */
                if (!vfs_free_sons(node, 1)) {
                    return 0;
                }

                if (ret == 0xffffffff) {
                    /* Media removed */
                    kerror("VFS: Media removed");
                }
            }
        }
    }


    /* If node is a mount point, return the ptr pointer */
    if (node->flags & VFS_FLAG_MOUNTPOINT) {
        /* Null? Read the root of the mountpoint which has
            this node as a rootdir parameter */
        if (!node->ptr) {
            uint64_t ino = node->inode;
            knotice("Cache flush");

            for (size_t m = 0; m < mount_count; m++) {
                char n[200];

                knotice("%s!!!!", n);
                if (mounts[m].root_dir == node &&
                    mounts[m].root_dir->inode == ino) {

                    vfs_get_full_path(mounts[m].root_dir, n);
                    knotice("Remounting %s at %s due to cache flushing",
                        mounts[m].dev->devname, n);

                    if (!mounts[m].fs->__vfs_get_root_dir(mounts[m].dev,
                            &mounts[m].root_dir->ptr)) {

                        kerror("VFS: couldn't read root dir, fs %s dev %s",
                        mounts[m].fs->fsname, mounts[m].dev->devname );
                        return 0;
                    } else {
                        *childs = mounts[m].root_dir->ptr;
                        vfs_node_t* node_childs = *childs;

                        while (node_childs) {
                            node_childs->mount = (void*)&mounts[m];
                            node_childs->parent = mounts[m].root_dir;
                            node_childs = node_childs->next;
                        }
                    }
                }


                if (m == mount_count) {
                    char n[128];
                    vfs_get_full_path(node, n);
                    kerror("Could not restore mount point in %s (unmounted?)",
                        n);
                    return 0;
                }



            }


        }

        *childs = node->ptr;

        if (!*childs)
            return 0;
        else
            return 1;

    } else {
        /* If node, return the childs pointer */

        if (!node->child) {
            if (node->__vfs_readdir) {
                if (!node->__vfs_readdir(node, childs))
                    return 0;
            } else {
                kerror("vfs_readdir() unsupported by this file");
                *childs = NULL;
                return 0;
            }

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
           // return base;
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
            if ((int)len < 0) {
                len = (uint32_t)node->size;
            }

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
void vfs_get_full_path(vfs_node_t* n, char* buf)
{
    char tmp[256];
    int i = 0;
    vfs_node_t* p = n;
    /* First, get all parent names */
    while (p != vfs_get_root()) {
        size_t l = strlen(p->name);
        memcpy(p->name, &tmp[i], l);
        i += l;
        tmp[i++] = '/';
        tmp[i] = 0;
        p = p->parent;
    }
    tmp[i] = 0;
    /* Then reverse it */
    char *tok = strrtok_n(tmp, '/', strlen(tmp)-3);
    size_t soff, eoff = strlen(tmp)-1;
    size_t idx = 0;
    int notok = 0;
    do {
        if (tok) {
            soff = (tok - tmp);
            tok = strrtok_n(tmp, '/', soff-1);
        } else {
            soff = 0;
            notok = 1;
        }
        memcpy(&tmp[soff], &buf[idx], (eoff - soff));
        knotice("<%s>", tmp);
        idx += (eoff-soff);
        if (notok) buf[idx++] = '/';
        eoff = soff;
    } while (!notok);
    buf[strlen(tmp)-1] = 0;
}
