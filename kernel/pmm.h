/*  Physical memory manager.

    Copyright (C) 2016 Arthur M
*/

#include <stdint.h>
#include <stddef.h>
#include <kstdlog.h>

#include "mmap.h"

#ifndef _PMM_H
#define _PMM_H

typedef uintptr_t physaddr_t;

/* 1 physical page = 4096 bytes */
#define PMM_PAGE_SIZE 4096

#define BITSET_SET(bitset, idx, bit) (bitset[idx] |= (1 << bit))
#define BITSET_ISSET(bitset, idx, bit) (bitset[idx] & (1 << bit))
#define BITSET_UNSET(bitset, idx, bit) (bitset[idx] &= ~(1 << bit))

/* Memory region types */
enum MemRegion {
    PMM_REG_DEFAULT = 0,    //  Default, ie usable RAM
    PMM_REG_HARDWARE = 1,   //  Hardware used RAM
    PMM_REG_LEGACY = 2,     //  RAM usable by legacy devices (ISA, old DMA),
                            // allocated below 1M
    PMM_REG_FAULTY = -1,    // RAM marked as bad.
};

struct MMAPRegion {
    physaddr_t start;
    size_t len;
    int region_type;

    uint8_t* region_bitset;
    physaddr_t first_free_addr;
};

/*  Initialize physical memory manager.
    Autodetect RAM regions, report usable physical RAM at end
*/
int pmm_init(struct mmap_list* mm_list, physaddr_t kstart,
    /*out*/ physaddr_t* kend);

/* Get memory usage from region */
size_t pmm_get_mem_total(int region);
size_t pmm_get_mem_used(int region);
size_t pmm_get_mem_free(int region);

physaddr_t pmm_alloc(size_t pages, int type);
physaddr_t pmm_reserve(physaddr_t addr, size_t pages);
int pmm_free(physaddr_t addr, size_t pages);

#endif /* end of include guard: _PMM_H */
