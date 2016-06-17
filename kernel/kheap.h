/* Kernel heap manager

    Copyright (C) 2016 Arthur M
*/

#include <stdarg.h>
#include <stddef.h>
#include <kstdlib.h>
#include "arch/i386/vmm.h"

#ifndef _KHEAP_H
#define _KHEAP_H

typedef struct _HeapItem {
    virtaddr_t addr;
    size_t bytes;
    uint32_t canary;
    uint32_t flags;

    struct _HeapItem* prev;
    struct _HeapItem* next;
} heap_item_t;

struct HeapList {
    heap_item_t* first;
    heap_item_t* last;
    size_t count;
};

void kheap_init();
virtaddr_t kheap_allocate(size_t bytes);
virtaddr_t kheap_allocate_phys(physaddr_t phys, size_t bytes);
void kheap_deallocate(virtaddr_t addr);

/* Allocate an item.
    This can mean create a new item or finding an existant item */
heap_item_t* _kheap_alloc_item(size_t bytes);

/* Find an item that represents the address 'addr'
    if mode & HFIND_NEAREST_ABOVE, return the nearest item bigger than the address
    if mode & HFIND_NEAREST_BELOW, return the nearest item smaller than the address */
heap_item_t* _kheap_find_item(struct HeapList* const list,
    virtaddr_t addr, int mode);

enum HeapFlags {
    HFLAGS_FREE = 0,
    HFLAGS_USED = 1,
    HFLAGS_PHYS = 2,
};

enum HeapFindMode {
    HFIND_NEAREST_ABOVE = 2,
    HFIND_NEAREST_BELOW = 4
};

void __kheap_dump(struct HeapList* const list);

#endif /* end of include guard: _KHEAP_H */
