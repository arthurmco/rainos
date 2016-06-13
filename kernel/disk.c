#include "disk.h"

static disk_t disks[MAX_DISKS];
int disk_count = 0;

int disk_add(struct disk* d)
{
    if (disk_count == MAX_DISKS)
        return -1;

    memcpy("disk\0\0", d->sysname, 7);
    utoa_s(disk_count, &d->sysname[4], 10);
    d->id = disk_count;

    /* Prepare device */
    device_t* dev = device_create(0x8000 | d->id, d->sysname,
        DEVTYPE_BLOCK | DEVTYPE_SEEKABLE, NULL);
    d->dev = dev;

    dev->b_size = (d->b_size == 0) ? 512 : d->b_size;

    knotice("DISK: Adding disk %s (%d MB) as %s",
        d->disk_name,
        (uint32_t)(d->b_size * d->b_count / 1048576) & 0xffffffff,
        d->sysname);

    disks[disk_count] = *d;
    return disk_count++;
}

void disk_remove(int id)
{
    disk_t* d = &disks[id];
    device_destroy(d->dev);
    knotice("DISK: Removing disk %s (%s, %d MB)",
        d->sysname,
        d->disk_name,
        (uint32_t)(d->b_size * d->b_count / 1048576) & 0xffffffff);
    memset(d, 0, sizeof(struct disk));
}

/* Get a disk by his system name (i.e disk0, disk1) */
disk_t* disk_get(const char* name)
{
    if (name[0] != 'd' || name[1] != 'i' || name[2] != 's' || name[3] != 'k')
    {
        return NULL;
    }

    int dnum = atoi(&name[4], 10);
    return &disks[dnum];
}

int disk_read(disk_t* d, uint64_t off, size_t len, void* buf)
{
    return d->__disk_read(d, off, len, buf);
}

int disk_write(disk_t* d, uint64_t off, size_t len, const void* buf)
{
    return d->__disk_write(d, off, len, buf);
}
