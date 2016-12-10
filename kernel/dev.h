/* Device abstraction header

    Copyright (C) 2015 Arthur M
*/

#include <stdint.h>
#include <stdarg.h>
#include <kstdlib.h>

#include "dev_ioctl.h"

#ifndef _DEV_H
#define _DEV_H

#define MAX_IOCTL_HANDLERS 8

enum DeviceType {
    DEVTYPE_BLOCK = 0x1,
    DEVTYPE_CHAR = 0x2,
    DEVTYPE_SEEKABLE = 0x4,
};

typedef int (*dev_ioctl_handler_f)(struct device*, uint16_t op, uint64_t* ret,
    uint32_t data1,  uint64_t data2);

/* The ioctl handler */
struct ioctl_handler {
    uint32_t ops[MAX_IOCTL_HANDLERS]; /* ops that will be sent to that ioctl */
    unsigned op_count;
    dev_ioctl_handler_f fun;    /* The ioctl function of the driver */
};

typedef struct device {
    uint64_t devid;
    char* devname;
    uint8_t devtype;
    char* devdesc;

    /* fill these if device is DEVTYPE_BLOCK */
    size_t b_size;
    int (*__dev_read)(struct device*, uint64_t off, size_t len, void* buf);
    int (*__dev_write)(struct device*, uint64_t off, size_t len,
        const void* buf);

    /* fill these if device is DEVTYPE_CHAR */
    int (*__dev_getc)(struct device*);
    int (*__dev_putc)(struct device*, int c);

    /* fill this if device is DEVTYPE_SEEKABLE */
    int (*__dev_seek)(struct device*, uint64_t off);

    /* fill this in any case */
    int (*__dev_ioctl)(struct device*, uint32_t op, uint64_t* ret,
        uint32_t data1,  uint64_t data2);

    /*  The new ioctl infrastructure.
        Better for drivers and me */
    struct ioctl_handler ioctls[MAX_IOCTL_HANDLERS];
    unsigned int ioctl_handler_count;

    struct device* next;
    struct device* prev;
    struct device* first_child;
    struct device* parent;
} device_t;

/*  Gets a device by ID or by name.
    Returns NULL if device hasn't been found */
device_t* device_get_by_id(uint64_t devid);
device_t* device_get_by_name(char* name);

/* Set a device description */
void device_set_description(device_t* dev, const char* desc);

/* Creates a device. Returns pointer */
device_t* device_create(uint64_t id, const char* name,
    uint8_t devtype, device_t* parent);

/* Destroys a device. */
void device_destroy(device_t* dev);

int device_read(device_t* dev, uint64_t off, size_t len, void* buf);
int device_ioctl(device_t* dev, uint32_t op, uint64_t* ret, uint32_t data1, uint64_t data2);

/* Add ioctl handler for operation op */
int device_add_ioctl_handler(device_t* dev, dev_ioctl_handler_f fun, uint32_t op);

//int device_add_ioctl_handler_multi(device_t* dev, dev_ioctl_handler_f fun, ...);


#endif /* end of include guard: _DEV_H */
