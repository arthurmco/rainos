#include "pages.h"

static pdir_t* dir_table;
static uintptr_t dir_table_phys;
static uintptr_t _kernel_vaddr;

void pages_init(uintptr_t dir_table_addr, uintptr_t kernel_vaddr)
{
    knotice("PAGES: directory table is at 0x%x", dir_table_addr);
    _kernel_vaddr = kernel_vaddr;

    /*  At this point, we're still using 1:1 addressing, so
        this can be fine to do */

    dir_table_phys = dir_table_addr;
    dir_table = (pdir_t*)dir_table_addr;
    dir_table[1023].addr = (dir_table_phys | 0x3);

    knotice("\tCreating recursive page mapping at 0x%x",
        &dir_table[1023]);

    /* Change the directory to the virtual address */
    dir_table = (pdir_t*)0xfffff000;

    ptable_t* page_table = (pdir_t*)dir_table;
    knotice("%x", page_table[1023].addr);

    knotice("\t(at virtaddr 0x%x) ",
        &dir_table[1023]);

}

pdir_t* page_dir_get(unsigned dir_index)
{
    if (dir_index >= MAX_DIR_COUNT)
        return NULL;

    if (dir_table[dir_index].options.present)
        return &dir_table[dir_index];
    else
        return NULL;
}

ptable_t* page_table_get(pdir_t* dir, unsigned table_index)
{
    /* Get the directory's page table address */
    uintptr_t ptable_addr = (dir->addr & ~0xfff);
    ptable_t* ptable = (ptable_t*)ptable_addr;
    return &ptable[table_index];
}

pdir_t* page_dir_create(unsigned dir, unsigned options)
{
    if (dir >= MAX_DIR_COUNT)
        return NULL;

    pdir_t* pdir = &dir_table[dir];

    /* create the table in the end of virtual memory */
    uintptr_t addr = pmm_alloc(1, PMM_REG_DEFAULT);
    pdir->addr = addr | (options & 0xfff);
    memset((void*)addr, 0, 4096);

    return pdir;
}

ptable_t* page_table_create(pdir_t* dir, unsigned table, physaddr_t addr,
    unsigned options)
    {
        if (!dir)
            return NULL;

        if (table >= MAX_TABLE_COUNT)
            return NULL;

        ptable_t* ptable = (ptable_t*)(dir->addr & ~0xfff);
        ptable[table].addr = addr | (options & 0xfff);
        return &ptable[table];
    }


physaddr_t page_dir_addr(pdir_t* dir)
{
    return (dir->addr & ~0xfff);
}
physaddr_t page_table_addr(ptable_t* table)
{

    return (table->addr & ~0xfff);
}
