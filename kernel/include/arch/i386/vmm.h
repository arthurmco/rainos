/*  Virtual memory manager
    Allocates and deallocate virtual memory areas for the user

    Copyright (C) 2016 Arthur M
    */

#include <stdint.h>
#include <stddef.h>
#include <kstdlog.h>

#include "pages.h"
#include "../../pmm.h"

#ifndef VMM_H
#define VMM_H

typedef uintptr_t virtaddr_t;

/*  Delimites some virtual address region.
    This is used for fast lookups */
struct virt_region_t {
    /* Minimum, maximum and first-free address of that region */
    virtaddr_t min_addr, max_addr, first_free_addr;
};

enum VMMAreas {
    VMM_AREA_USER = 1,   // User virtual area, 0x0 -> 0xbfffffff
    VMM_AREA_KERNEL = 0, // Kernel virtual area, 0xc0000000 -> 0xffffffff
    VMM_AREA_COUNT = 2
};

#define GET_DIR(addr) (addr >> 22)
#define GET_TABLE(addr) ((addr >> 12) & 0x3ff)

#define VMM_PAGE_SIZE 4096

/* Allocate 'count' pages from virtual space*/
virtaddr_t vmm_alloc_page(unsigned int vmm_area, size_t count);

/*  Deallocate pages .
    This usually means set the present bit to 0 */
void vmm_dealloc_page(virtaddr_t addr, size_t count);

/*  Map 'count' pages to 'addr' and return 'addr' or 0 on fail */
virtaddr_t vmm_map_page(virtaddr_t addr, unsigned int vmm_area,
    size_t count);

/*  Alloc 'count' pages and store their physaddr on
    the 'phys' variable */
virtaddr_t vmm_alloc_physical(unsigned int vmm_area,
    physaddr_t* phys, size_t count, int pmm_type);

/*  Map 'count' pages and map these pages to physical address
        'phys' . You need to allocate these address first!*/
virtaddr_t vmm_map_physical(unsigned int vmm_area,
    physaddr_t phys, size_t count);

void vmm_init(uintptr_t kernel_start, uintptr_t kernel_end,
    uintptr_t kernel_virt_offset);
#endif /* end of include guard: VMM_H */
