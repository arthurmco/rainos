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


/* Partition ioctls *********/

/* Returns the starting LBA of the partition, in ret pointer */
#define IOCTL_LBA_START 0x81

/* Returns the size LBA of the partition */
#define IOCTL_LBA_SIZE 0x82

/* Sets the used space. Put it in data2 (the 64-bit parameter)  */
#define IOCTL_SET_USED_SPACE 0x83

/* Get the used space, in ret pointer */
#define IOCTL_GET_USED_SPACE 0x84

/* Sets the label. Put the pointer in data1 and length in data2 */
#define IOCTL_SET_LABEL 0x85

/* Gets the label, as a 64-bit zeroextended pointer in ret. */
#define IOCTL_GET_LABEL 0x86

/* Sets the fsname, same way as the label */
#define IOCTL_SET_FSNAME 0x88

/* Sets the mount status: 0 for unmounted, 1 for mounted */
#define IOCTL_SET_MOUNTED 0x90

#endif
