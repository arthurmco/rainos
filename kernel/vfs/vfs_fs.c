#include <vfs/vfs.h>

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

vfs_mount_t mounts[MAX_MOUNTS];
unsigned mount_count = 0;

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
