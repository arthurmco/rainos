#include "disk.h"

static disk_t disks[MAX_DISKS];
int disk_count = 0;

static int _disk_dev_read_wrapper(device_t* dev,
    uint64_t off, size_t len, void* buf)
{
    disk_t* disk = NULL;
    for (size_t i = 0; i < MAX_DISKS; i++)
    {
        disk = &disks[i];
        if (disk->dev == dev || disk->dev->devid == dev->devid)
        {
            knotice("\t disk: found disk %s = %s", disk->dev->devname, dev->devname);

            /*  Here we convert offset and lengths, that are in bytes, in
                sectors */

            uint64_t off_sector;
            size_t len_sector;
            size_t byte_start_spare = off % disk->b_size;

            off_sector = off / disk->b_size;
            len_sector = 1 + ((len - 1 + byte_start_spare) / disk->b_size);

            knotice("\t disk: converted off=%d, len=%d in bytes "
                "to off=%d, len=%d in sectors, using %d byte sectors",
                (uint32_t)(off & 0xffffffff), (uint32_t)(len & 0xffffffff),
                (uint32_t)(off_sector & 0xffffffff),
                (uint32_t)(len_sector & 0xffffffff), disk->b_size);

            void* newbuf = kmalloc(len_sector*disk->b_size);

            int ret = disk_read(disk, off_sector, len_sector, newbuf);

            memcpy(&newbuf[byte_start_spare], buf,
                (len_sector * disk->b_size) - byte_start_spare);

            return 0;
        }
    }

    // Not found.
    return -1;
}


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

    d->b_size = (d->b_size == 0) ? 512 : d->b_size;
    dev->b_size = d->b_size;
    dev->__dev_read = &_disk_dev_read_wrapper;

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
