#include "pages.h"

static pdir_t* dir_table;
static uintptr_t _kernel_vaddr;
void pages_init(uintptr_t dir_table_addr, uintptr_t kernel_vaddr)
{
    knotice("PAGES: directory table is at 0x%x", dir_table_addr);
    dir_table = (pdir_t*)dir_table_addr;
    _kernel_vaddr = kernel_vaddr;
}

pdir_t* page_dir_get(unsigned dir_index)
{
    if (dir_index >= MAX_DIR_COUNT)
        return NULL;

    return &dir_table[dir_index];
}

ptable_t* page_table_get(pdir_t* dir, unsigned table_index)
{
    /* Get the directory's page table address */
    uintptr_t ptable_addr = (dir->addr & ~0x3ff);
    ptable_t* ptable = (ptable_t*)ptable_addr;
    return &ptable[table_index];
}

pdir_t* page_dir_create(unsigned dir, unsigned options)
{
    if (dir >= MAX_DIR_COUNT)
        return NULL;

    pdir_t* pdir = &dir_table[dir];
    uintptr_t addr = pmm_alloc(1, PMM_REG_DEFAULT);
    pdir->addr = addr | (options & 0x3ff);
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

        ptable_t* ptable = (ptable_t*)(dir->addr & ~0x3ff);
        ptable[table].addr = addr | (options & 0x3ff);
        return &ptable[table];
    }


physaddr_t page_dir_addr(pdir_t* dir)
{
    return (dir->addr & ~0x3ff)
}
physaddr_t page_table_addr(ptable_t* table)
{

    return (table->addr & ~0x3ff)
}
