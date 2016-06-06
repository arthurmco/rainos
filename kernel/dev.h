/* Device abstraction header

    Copyright (C) 2015 Arthur M
*/

#include <stdint.h>
#include <stdarg.h>

#ifndef _DEV_H
#define _DEV_H


typedef struct device {
    uint64_t devid;
    char* devname;

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
device_t* device_create(uint64_t id, char* name,
    device_t* parent);

/* Destroys a device. */
void device_destroy(device_t* dev);


#endif /* end of include guard: _DEV_H */
