#include "partition.h"

static device_t* devices[MAX_DEVICES];
static unsigned dev_count;

static struct partition partitions[MAX_DEVICES][MAX_PARTITIONS];

static int vfs_p_count = 0;
static struct vfs_partition vfs_partitions[MAX_PARTITIONS*MAX_DEVICES];

static unsigned part_count[MAX_DEVICES];

static int __partition_dev_read(device_t* dev, uint64_t off,
    size_t len, void* buf);
static int __partition_dev_ioctl(device_t* dev, uint32_t op, uint64_t* ret,
    uint32_t data1, uint64_t data2);

/* TODO: fix this crap.
    This hack is to show partitions inside corresponding disks in lsdisks command*/
int partitions_cmd(device_t* dev) {
    int hasPart = 0;
    for (int i = 0; i < vfs_p_count; i++) {
        if (dev->devid == vfs_partitions[i].dev->parent->devid) {
            hasPart++;
            kprintf("\t%s: %s ", vfs_partitions[i].dev->devname,
                vfs_partitions[i].name);

            if (vfs_partitions[i].mounted) {
                kprintf("\n\t\t Start: %u sectors | Size: %u sectors\n",
                    (uint32_t)(vfs_partitions[i].lba_start & 0xffffffff),
                    (uint32_t)(vfs_partitions[i].lba_size & 0xffffffff));
                kprintf("\t\t Filesystem: %s\n",
                    vfs_partitions[i].fsname);
            } else {
                kprintf("[Not mounted]\n");
            }

        }
    }

    if (!hasPart) {
        kprintf("\t No partitions\n");
    } else {
        kprintf("\n");
    }
}

/* Initialize partition parsing system */
void partitions_init()
{
    knotice("PARTITIONS: partition support started");
    dev_count = 0;

    char* dname = "disk0";
    int i = 0;
    device_t* dev = NULL;

    /* Autodetect the disks and its partitions */
    do {
        dname[4] = '0'+i;
        knotice("partitions: checking %s", dname);

        dev = device_get_by_name(dname);

        if (dev) {
            partitions_retrieve(dev);
            dev_count++;
        }
        i++;
    } while (i < 10);

}

/*  Retrieve partitions from a device.
    It needs to be a block device,
        (maybe in the future support char device w/ seeking)

    Returns the count of partitions, or -1 if error

        The partitions will be created at [device_name]p[partition_num],
    staring at 1
        Ex: If we pass disk0 as the device, this function will create
    disk0p1, disk0p2, disk0p3...
*/
int partitions_retrieve(device_t* dev)
{

    if (!dev) return -1;

    unsigned nid = -1;
    /* check if device wasn't mapped */
    for (unsigned i = 0; i < dev_count; i++)
    {
        if (devices[i]->devid == dev->devid)
            nid = i;
    }

    if (nid == (unsigned int)-1)
        nid = dev_count;

    void* partition = kcalloc(512,1);

    if (!device_read(dev, 0, 512, partition)) {
        kerror("PARTITION: could not read device %s", dev->devname);
        return -1;
    }

    struct partition* part = (struct partition*)(((uint32_t)partition)+0x01be);

    devices[nid] = dev;

    /* Check if partitions are valid */
    unsigned max_parts = 0;
    for (unsigned i = 0; i < 4; i++) {
        if (part[i].system_id != 0 &&
            part[i].lba_size > 0) {
                char partname[strlen(dev->devname)*2];
                sprintf(partname, "%sp%d", dev->devname, max_parts+1);
                knotice("PARTITIONS: Discovered partition %s\n"
                    "\tlba %d, %d sectors, type 0x%02x",
                    partname,
                    part[i].lba_start, part[i].lba_size,
                    part[i].system_id);
                partitions[nid][max_parts] = *part;

                /* TODO: Make 'dev' the parent device */
                device_t* partdev = device_create(
                    (dev->devid << 8) | (max_parts << 4) | nid,
                        partname, DEVTYPE_BLOCK, NULL);
                partdev->__dev_read = &__partition_dev_read;
                partdev->__dev_ioctl = &__partition_dev_ioctl;
                partdev->parent = dev;

                vfs_partitions[vfs_p_count].lba_start = part[i].lba_start;
                vfs_partitions[vfs_p_count].lba_size = part[i].lba_size;
                vfs_partitions[vfs_p_count].lba_used_space = 0;
                vfs_partitions[vfs_p_count].dev = partdev;
                vfs_partitions[vfs_p_count].mounted = 0;
                vfs_p_count++;
                max_parts++;
        }
    }

    part_count[nid] = max_parts;


    knotice("PARTITIONS: %d partitions on %s",
        max_parts, dev->devname);
    return max_parts;

}

/* Retrieve the device of the corresponding partition number
    (usually disk0p{num}), or NULL if partition doesn't exist */
device_t* partitions_get_device(device_t* disk, int num)
{
    int devid = 0;
    for (int i = 0; i < dev_count; i++) {
        if (devices[i]->devid == disk->devid) {
            devid = i;
            break;
        }
    }

    char dname[strlen(disk->devname)+6];
    sprintf(dname, "%sp%d", disk->devname, num);
    return device_get_by_name(dname);
}


static int __partition_dev_read(device_t* dev, uint64_t off,
    size_t len, void* buf)
    {
        unsigned nid = (dev->devid & 0xffff);
        unsigned part = (dev->devid >> 4) & 0xffff;

        if (nid > dev_count) {
            kerror("partition: invalid read to %s, unexistant parent device",
                dev->devname);
            return -1;
        }

        if (part > part_count[nid]) {
            kerror("partition: invalid read to %s, unexistant partition number %d at %s",
                dev->devname, part, devices[nid]->devname);
            return -1;
        }

        uint64_t new_off = (partitions[nid][part].lba_start * 512);

        if (len > (partitions[nid][part].lba_size * 512)) {
            kwarn("partition: trying to read more than partition size at %s",
                devices[nid]->devname);
            len = (partitions[nid][part].lba_size * 512);
        }

        knotice("partition: off=%d turned into off=%d", off, new_off+off);
        int r = devices[nid]->__dev_read(devices[nid], new_off+off, len, buf);
        return r;
    }

static int __partition_dev_ioctl(device_t* dev, uint32_t op, uint64_t* ret,
    uint32_t data1, uint64_t data2)
    {
        /* Identify the partition */
        struct vfs_partition* p = NULL;
        for (int i = 0; i < vfs_p_count; i++) {
            if (vfs_partitions[i].dev->devid == dev->devid) {
                p = &vfs_partitions[i];
                break;
            }
        }

        if (!p) return 0;

        switch (op) {
            case IOCTL_LBA_START:
                *ret = p->lba_start;
                return 1;

            case IOCTL_LBA_SIZE:
                *ret = p->lba_size;
                return 1;

            case IOCTL_SET_USED_SPACE:
                p->lba_used_space = data2;
                return 1;

            case IOCTL_GET_USED_SPACE:
                *ret = p->lba_used_space;
                return 1;

            case IOCTL_GET_LABEL:
                *ret = (uint64_t)p->name;
                return 1;

            case IOCTL_SET_LABEL:
            {
                char* lbl = (char*)data1;
                memcpy(lbl, p->name, data2 > 32 ? 32 : data2);
                return 1;
            }

            case IOCTL_SET_FSNAME:
            {
                char* f = (char*)data1;
                memcpy(f, p->fsname, data2 > 16 ? 16 : data2);
                return 1;
            }

            case IOCTL_SET_MOUNTED:
                p->mounted = data1;
                return 1;



        }

        return -1;
    }
