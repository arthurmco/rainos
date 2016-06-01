/* Memory map generic definitions

    Copyright (C) 2016 Arthur M
*/

#include <stdint.h>
#include <stdarg.h>

#ifndef _MMAP_H
#define _MMAP_H

typedef struct mmap {
    uintptr_t start;
    size_t length;
    int type;
} mmap_t;

struct mmap_list {
    mmap_t* maps;
    int size;
};

#endif /* end of include guard: _MMAP_H */
