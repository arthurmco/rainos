/*  Initial ram disk support

    Copyright (C) 2016 Arthur M
*/

#include <stdint.h>
#include <stddef.h>
#include <kstdlog.h>

#include "vfs/vfs.h"

#ifndef _INITRD_H
#define _INITRD_H

struct initrd {
    uintptr_t start, end;
};

/*  Initialize the initrd
    Returns 1 if succeded, 0 if not */
int initrd_init(uintptr_t start, uintptr_t end);

/* Verify the initrd checksum
    1 if ok, 0 if not
*/
int initrd_verify_checksum(struct initrd* rd);

/* Register initrd filesystem in the vfs */
void initrd_register();


#endif /* end of include guard: _INITRD_H */
