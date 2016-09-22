/* Virtual page manipulation for RainOS

    Copyright (C) 2016 Arthur M */

#include <stdint.h>
#include <kstdlog.h>
#include "../../pmm.h"

#ifndef _PAGES_H
#define _PAGES_H

#define MAX_DIR_COUNT 1024
#define MAX_TABLE_COUNT 1024

typedef union page_dir {
    uint32_t addr;
    struct dir_opt{
        unsigned present:1;
        unsigned writable:1;
        unsigned user:1;
        unsigned write_through:1;
        unsigned no_cache:1;
        unsigned accessed:1;
        unsigned zero:1;
        unsigned dir_4mb:1;
        unsigned global:1;
        unsigned avail:3;
        unsigned dir_location:20;
    } options;
} pdir_t;

typedef union page_table {
    uint32_t addr;
    struct table_opt{
        unsigned present:1;
        unsigned writable:1;
        unsigned user:1;
        unsigned write_through:1;
        unsigned no_cache:1;
        unsigned accessed:1;
        unsigned dirty:1;
        unsigned zero:1;
        unsigned global:1;
        /*  RainOS specific: if 1, means that this page is part of an allocation
            chain, and there's more pages after */
        unsigned chained_next:1;

        /*  RainOS specific: if 1, means that this page is part of an allocation
            chain, and there's more pages before */
        unsigned chained_prev:1;
        unsigned avail:1;
        unsigned dir_location:20;
    } options;
} ptable_t;

/* Page table virtual to physical translation structs */
struct table_virt {
    unsigned dir, table;
    physaddr_t phys;
    virtaddr_t virt;
    struct table_virt *prev, *next;
};

void pages_init(uintptr_t dir_table_addr, uintptr_t kernel_vaddr);

pdir_t* page_dir_get(unsigned dir_index);
ptable_t* page_table_get(pdir_t* dir, unsigned table_index);

pdir_t* page_dir_create(unsigned dir, unsigned options);
ptable_t* page_table_create(pdir_t* dir, unsigned table, physaddr_t addr,
    unsigned options);

physaddr_t page_dir_addr(pdir_t* dir);
physaddr_t page_table_addr(ptable_t* table);


#endif /* end of include guard: _PAGES_H */
