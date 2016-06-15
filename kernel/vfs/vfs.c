#include "vfs.h"

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

int vfs_register_fs(vfs_filesystem_t* fs)
{
    return 0;
}

vfs_filesystem_t* vfs_get_fs(const char* name)
{
    return 0;
}

/*  Mount the device 'dev' as the filesystem 'fs' in the 'node' node.
    'node' will become the new filesystem root */
int vfs_mount(vfs_node_t* node, device_t* dev, struct vfs_filesystem* fs)
{
    return 0;
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
        /* Null? Read the root of the mountpoint which hace
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