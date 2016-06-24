#include "partition.h"

static device_t* devices[MAX_DEVICES];
static unsigned dev_count;

static struct partition partitions[MAX_DEVICES][MAX_PARTITIONS];
static unsigned part_count[MAX_DEVICES];

static int __partition_dev_read(device_t* dev, uint64_t off,
    size_t len, void* buf);

/* Initialize partition parsing system */
void partitions_init()
{
    knotice("PARTITIONS: partition support started");
    dev_count = 0;
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
                device_t* part = device_create(
                    (dev->devid << 8) | (max_parts << 4) | nid,
                        partname, DEVTYPE_BLOCK, NULL);
                part->__dev_read = &__partition_dev_read;
                max_parts++;
        }
    }

    part_count[nid] = max_parts;
    knotice("PARTITIONS: %d partitions on %s",
        max_parts, dev->devname);
    return max_parts;

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
