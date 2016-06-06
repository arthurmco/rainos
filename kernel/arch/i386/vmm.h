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

enum VMMAreas {
    VMM_AREA_USER = 1,   // User virtual area, 0x0 -> 0xbfffffff
    VMM_AREA_KERNEL = 0, // Kernel virtual area, 0xc0000000 -> 0xffffffff
    VMM_AREA_COUNT = 2
};

#define VMM_PAGE_SIZE 4096

/* Allocate 'count' pages from virtual space*/
virtaddr_t vmm_alloc_page(unsigned int vmm_area, size_t count);

/*  Deallocate pages .
    This usually means set the present bit to 0 */
void vmm_dealloc_page(virtaddr_t addr, size_t count);

/*  Alloc 'count' pages and map these pages to physical address
    'phys' */
virtaddr_t vmm_alloc_physical(unsigned int vmm_area,
    physaddr_t phys, size_t count);

void vmm_init(uintptr_t kernel_start, uintptr_t kernel_end,
    uintptr_t kernel_virt_offset);
#endif /* end of include guard: VMM_H */
