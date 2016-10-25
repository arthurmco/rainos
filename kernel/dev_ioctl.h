/*  Common ioctl codes

    Copyright (C) 2016 Arthur M
*/

#ifndef _DEV_IOCTL_H
#define _DEV_IOCTL_H

/*** WARNING: Custom IOCTLs should take numbers over 0x8000 (32768) */

/* Ask if the media was changed
    Can be useful for CDs

    If the media changed, *ret = 1.
    If no media, *ret = 0xffffffff
    Otherwise, *ret is unchanged.
*/
#define IOCTL_CHANGED   0x1



#endif
