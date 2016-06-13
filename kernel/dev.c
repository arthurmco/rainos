#include "dev.h"

static device_t* last_dev = NULL;


/*  Gets a device by ID or by name.
    Returns NULL if device hasn't been found */
device_t* device_get_by_id(uint64_t devid)
{
    device_t* d = last_dev;

    while (d)
    {
        if (d->devid == devid)
        {
            return d;
        }

        d = d->prev;
    }

    return NULL;
}

device_t* device_get_by_name(char* name)
{
    device_t* d = last_dev;

    while (d)
    {
        if (!strncmp(d->devname, name, strlen(name)))
        {
            return d;
        }

        d = d->prev;
    }

    return NULL;
}

/* Creates a device. Returns pointer */
device_t* device_create(uint64_t id, const char* name,
    uint8_t devtype, device_t* parent)
    {
        device_t* dev = kcalloc(sizeof(device_t), 1);
        dev->devname = kmalloc(strlen(name));
        dev->devtype = devtype;
        memcpy(name, dev->devname, strlen(name));
        dev->devid = id;
        dev->parent = parent;
        knotice("DEV: Created device %s (id 0x%x%x), type 0x%x, child of %s",
            dev->devname, (uint32_t)(id>>32), (uint32_t)(id&0xffffffff),
            dev->devtype, (parent) ? (parent->devname) : "<NULL>");

        if (last_dev && !parent) {
            dev->prev = last_dev;
            last_dev->next = dev;
        }

        if (parent) {
            if (!parent->first_child) {
                parent->first_child = dev;
            } else {

                dev->prev = parent->first_child;
                dev->next = parent->first_child->next;
                parent->first_child->next = dev;
            }

        }


        last_dev = dev;
        return dev;
    }

/* Destroys a device. */
void device_destroy(device_t* dev)
{
    dev->next->prev = dev->prev;
    dev->prev->next = dev->next;

    kfree(dev->devname);
    kfree(dev);
}

int device_read(device_t* dev, uint64_t off, size_t len, void* buf)
{
    if (!dev) return 0;
    return dev->__dev_read(dev, off, len, buf);
}