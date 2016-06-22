/*  Initial ram disk support

    Copyright (C) 2016 Arthur M
*/

#include <stdint.h>
#include <stddef.h>
#include <kstdlog.h>

#include "vfs/vfs.h"

#ifndef _INITRD_H
#define _INITRD_H

typedef struct initrd {
    uintptr_t start, end;
} initrd_t;

struct tar_block_header
{                            //byte offset//
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */

};

struct initrd_file {
    char name[64];
    size_t size;
    uintptr_t addr;
    struct initrd_file* next;
    struct initrd_file* prev;
    struct initrd_file* parent;
    struct initrd_file* child;
};

/*  Initialize the initrd
    Returns 1 if succeded, 0 if not */
int initrd_init(uintptr_t start, uintptr_t end);

/* Verify the initrd checksum
    1 if ok, 0 if not
*/
int initrd_verify_checksum(struct tar_block_header* hdr);

/* Register initrd filesystem in the vfs */
void initrd_register();

/* Mount initrd direcly over root */
int initrd_mount();

#endif /* end of include guard: _INITRD_H */
