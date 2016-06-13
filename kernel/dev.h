/* Device abstraction header

    Copyright (C) 2015 Arthur M
*/

#include <stdint.h>
#include <stdarg.h>
#include <kstdlib.h>

#ifndef _DEV_H
#define _DEV_H

enum DeviceType {
    DEVTYPE_BLOCK = 0x0,
    DEVTYPE_CHAR = 0x1,
    DEVTYPE_SEEKABLE = 0x2,
};

typedef struct device {
    uint64_t devid;
    char* devname;
    uint8_t devtype;

    /* fill these if device is DEVTYPE_BLOCK */
    size_t b_size;
    int (*__dev_read)(struct device*, size_t len, void* buf);
    int (*__dev_write)(struct device*, size_t len, const void* buf);

    /* fill these if device is DEVTYPE_CHAR */
    int (*__dev_getc)(struct device*);
    int (*__dev_putc)(struct device*, int c);

    /* fill this if device is DEVTYPE_SEEKABLE */
    int (*__dev_seek)(struct device*, uint64_t off);

    uint64_t (*__dev_tell)(struct device*);

    struct device* next;
    struct device* prev;
    struct device* first_child;
    struct device* parent;
} device_t;

/*  Gets a device by ID or by name.
    Returns NULL if device hasn't been found */
device_t* device_get_by_id(uint64_t devid);
device_t* device_get_by_name(char* name);

/* Creates a device. Returns pointer */
device_t* device_create(uint64_t id, const char* name,
    uint8_t devtype, device_t* parent);

/* Destroys a device. */
void device_destroy(device_t* dev);


#endif /* end of include guard: _DEV_H */
