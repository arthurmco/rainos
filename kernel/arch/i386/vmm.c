#include "vmm.h"

/* Stores the maximum address allocated for these areas */
static virtaddr_t max_virt[VMM_AREA_COUNT];
static virtaddr_t cur_virt[VMM_AREA_COUNT];


void vmm_init(uintptr_t kernel_start, uintptr_t kernel_end,
    uintptr_t kernel_virt_offset)
    {
        /* TODO: remove this when you go to higher half */
        kernel_virt_offset = 0xc0000000;

        max_virt[VMM_AREA_USER] = kernel_end + VMM_PAGE_SIZE;
        max_virt[VMM_AREA_KERNEL] = (kernel_virt_offset + kernel_end);

        cur_virt[VMM_AREA_USER] = max_virt[VMM_AREA_USER];
        cur_virt[VMM_AREA_KERNEL] = max_virt[VMM_AREA_KERNEL];
    }

/*  Check if some pages are allocated.
    Outputs next non-allocated dir and table */
static int vmm_check_if_page_allocated(unsigned int* dir, unsigned int* table,
    size_t pages)
    {
        unsigned int iDir = (*dir);
        unsigned int iTable = (*table);
        int checked = 0;
        size_t checkedPages = 0;

        unsigned startingPage = iTable;
        for (int d = 0; d < MAX_DIR_COUNT-iDir; d++) {
            pdir_t* pdir = page_dir_get(iDir+d);

            if (!pdir || (!pdir->addr)) {
                pdir = page_dir_create(iDir+d, 0x3);
            }

            for (int t = startingPage; t < MAX_TABLE_COUNT-iTable; t++) {

                ptable_t* tbl = page_table_get(pdir, iTable+t);
    //            knotice("check iDir:%d d:%d iTable:%d t:%d checkedPages:%d "
    //                    "content: 0x%x",
    //                iDir, d, iTable, t, checkedPages, ((tbl) ? tbl->addr : 0));
                if (!tbl)
                    checkedPages++;
                else if (!tbl->options.present) {
                    checkedPages++;
                } else if (!tbl->options.dir_location) {
                    checkedPages++;
                } else {
                    checkedPages = 0;
                }


                if (checkedPages >= (pages)) {
                    checked = 1;
                    t++;
                    iTable = (t - pages);

                    t -= (pages);
                    if (t < 0) {
                        iTable = (1024 + (t));
                        d--;
                    }
                    t += (pages);
                    t--;


    //                knotice("Directories %d %d -> %d %d (t:%d p:%d)",
//                        (*dir), (*table), iDir, iTable, t, pages);

                    break;
                }
            }

            if (checked) {
                iDir += d;
                break;
            }
            iTable = 0;
            startingPage = 0;
        }

    //    knotice("Directories %d %d -> %d %d",
//            (*dir), (*table), iDir, iTable);

        (*dir) = iDir;
        (*table) = iTable;

        return checked;
    }

/* Allocate 'count' pages from virtual space*/
virtaddr_t vmm_alloc_page(unsigned int vmm_area, size_t count)
{
    if (vmm_area >= VMM_AREA_COUNT) {
        return NULL;
    }

    virtaddr_t addr = max_virt[vmm_area];
    unsigned dir = (addr >> 22) & 0x3ff;
    unsigned page = (addr >> 12) & 0x3ff;
    knotice("actual dir = %x, %x (%x)", dir, page, addr);

    if (!vmm_check_if_page_allocated(&dir, &page, count)) {
        return NULL; // No more pages.
    }

    addr = ((dir << 22) | (page << 12));

    pdir_t* pdir = page_dir_get(dir);

    if (!pdir)
        pdir = page_dir_create(dir, 0x3);
    else if (!pdir->addr)
        pdir = page_dir_create(dir, 0x3);
    else if (!pdir->options.present)
        pdir = page_dir_create(dir, 0x3);
    else if (!pdir->options.dir_location)
        pdir = page_dir_create(dir, 0x3);

    if (vmm_area == VMM_AREA_USER)
        pdir->options.user = 1;

    //knotice("pdir %d at 0x%x, addr 0x%x", dir, pdir,
    //    pdir->addr);

    /* Create the pages */
    for (size_t i = 0; i < count; i++) {
        ptable_t* ptable = page_table_get(pdir, page+i);

        if (!ptable)
            ptable = page_table_create(pdir, page+i,
                pmm_alloc(1, PMM_REG_DEFAULT), 0x3);
        else if (!ptable->options.present)
            ptable = page_table_create(pdir, page+i,
                pmm_alloc(1, PMM_REG_DEFAULT), 0x3);
        else if (!ptable->options.dir_location)
            ptable = page_table_create(pdir, page+i,
                pmm_alloc(1, PMM_REG_DEFAULT), 0x3);
        else
            ptable->options.dir_location = (pmm_alloc(1, PMM_REG_DEFAULT) >> 12);


        if (i > 0)
            ptable->options.chained_prev = 1;

        if (i < (count-1)) {
            ptable->options.chained_next = 1;
        }

        if (vmm_area == VMM_AREA_USER)
            ptable->options.user = 1;

    //    knotice("page %d: 0x%x 0x%x", page+i, ptable, ptable->addr);

        if (page+i > MAX_TABLE_COUNT) {
            page = 0;
            i = 0;
            count -= i;

            dir++;
            pdir = page_dir_get(dir);

            if (!pdir->options.present)
                pdir = page_dir_create(dir, 0x3);

            if (vmm_area == VMM_AREA_USER)
                pdir->options.user = 1;

        }
    }

    if (max_virt[vmm_area] < addr)
      max_virt[vmm_area] += (count * VMM_PAGE_SIZE);

    knotice("VMM: Allocated %d pages at virtaddr 0x%x", count, addr);
    return addr;
}

/*  Deallocate pages .
    This usually means set the present bit to 0 */
void vmm_dealloc_page(virtaddr_t addr, size_t count)
{
    unsigned dir = (addr >> 22) & 0x3ff;
    unsigned page = (addr >> 12) & 0x3ff;
    pdir_t* pdir = page_dir_get(dir);

    for (unsigned i = 0; i < count; i++) {
        ptable_t* ptable = page_table_get(pdir, page+i);
        physaddr_t phys = (ptable->addr & ~0x3ff);
        if (phys > 0)
            pmm_free(phys, 1);

        ptable->options.present = 0;
        ptable->options.chained_next = 0;
        ptable->options.chained_prev = 0;

        if (page+i > MAX_TABLE_COUNT) {
            page = 0;
            i = 0;
            count -= i;

            dir++;
            pdir = page_dir_get(dir);

        }
    }

    int a;

    if (addr < max_virt[VMM_AREA_KERNEL])
        a = VMM_AREA_USER;
    else
        a = VMM_AREA_KERNEL;


    knotice("VMM: Disallocated %d pages at virtaddr 0x%x", count, addr);
    max_virt[a] = addr;

}

/*  Alloc 'count' pages and store their physaddr on
    the 'phys' variable */
virtaddr_t vmm_alloc_physical(unsigned int vmm_area,
    physaddr_t* phys, size_t count, int pmm_type)
    {
        if (vmm_area >= VMM_AREA_COUNT) {
            return NULL;
        }

        virtaddr_t addr = max_virt[vmm_area];
        unsigned dir = (addr >> 22) & 0x3ff;
        unsigned page = (addr >> 12) & 0x3ff;
        knotice("(phys) actual dir = %x, %x (%x)", dir, page, addr);

        if (!vmm_check_if_page_allocated(&dir, &page, count)) {
            return NULL; // No more pages.
        }

        addr = ((dir << 22) | (page << 12));
        pdir_t* pdir = page_dir_get(dir);


        if (!pdir)
            pdir = page_dir_create(dir, 0x3);
        else if (!pdir->addr)
            pdir = page_dir_create(dir, 0x3);
        else if (!pdir->options.present)
            pdir = page_dir_create(dir, 0x3);
        else if (!pdir->options.dir_location)
            pdir = page_dir_create(dir, 0x3);


        if (vmm_area == VMM_AREA_USER)
            pdir->options.user = 1;

        //knotice("pdir %d at 0x%x, addr 0x%x", dir, pdir,
        //pdir->addr);

        /* Create the pages */
        *phys = pmm_alloc(count, pmm_type);
        physaddr_t allocPhys = *phys;
        for (size_t i = 0; i < count; i++) {
            ptable_t* ptable = page_table_get(pdir, page+i);

            //knotice("-- %d pt 0x%x ", i, ptable);

            if (!ptable)
                ptable = page_table_create(pdir, page+i, allocPhys, 0x3);
            else if (!ptable->options.present)
                ptable = page_table_create(pdir, page+i, allocPhys, 0x3);
            else if (!ptable->options.dir_location)
                ptable = page_table_create(pdir, page+i, allocPhys, 0x3);
            else
                ptable->options.dir_location = (allocPhys >> 12);

            if (i >= 0)
                ptable->options.chained_prev = 1;

            if (i < (count-1)) {
                ptable->options.chained_next = 1;
            }

            if (vmm_area == VMM_AREA_USER)
                ptable->options.user = 1;

            //knotice("page %d\\%d: 0x%x 0x%x paddr 0x%x", page, i, ptable, ptable->addr,
            //allocPhys);

            if (page+i > MAX_TABLE_COUNT) {
                page = 0;
                i = 0;
                count -= i;

                dir++;
                pdir = page_dir_get(dir);

                if (!pdir->options.present)
                    pdir = page_dir_create(dir, 0x3);

                if (vmm_area == VMM_AREA_USER)
                    pdir->options.user = 1;

            }

            allocPhys += VMM_PAGE_SIZE;
        }


        max_virt[vmm_area] += (count * VMM_PAGE_SIZE);

        knotice("VMM: Allocated %d pages at virtaddr 0x%x, "
            "mapped to physaddr 0x%x", count, addr, phys);
        return addr;
    }

/*  Map 'count' pages and map these pages to physical address
        'phys' . You need to allocate these address first!*/
virtaddr_t vmm_map_physical(unsigned int vmm_area,
    physaddr_t phys, size_t count)
    {
        if (vmm_area >= VMM_AREA_COUNT) {
            return NULL;
        }

        virtaddr_t addr = max_virt[vmm_area];
        unsigned dir = (addr >> 22) & 0x3ff;
        unsigned page = (addr >> 12) & 0x3ff;
        knotice("(phys) actual dir = %x, %x (%x)", dir, page, addr);

        if (!vmm_check_if_page_allocated(&dir, &page, count)) {
            return NULL; // No more pages.
        }

        addr = ((dir << 22) | (page << 12));
        pdir_t* pdir = page_dir_get(dir);


        if (!pdir)
            pdir = page_dir_create(dir, 0x3);
        else if (!pdir->addr)
            pdir = page_dir_create(dir, 0x3);
        else if (!pdir->options.present)
            pdir = page_dir_create(dir, 0x3);
        else if (!pdir->options.dir_location)
            pdir = page_dir_create(dir, 0x3);


        if (vmm_area == VMM_AREA_USER)
            pdir->options.user = 1;

        //knotice("pdir %d at 0x%x, addr 0x%x", dir, pdir,
        //pdir->addr);

        /* Create the pages */

        physaddr_t allocPhys = phys;
        for (size_t i = 0; i < count; i++) {
            ptable_t* ptable = page_table_get(pdir, page+i);

            //knotice("-- %d pt 0x%x ", i, ptable);

            if (!ptable)
                ptable = page_table_create(pdir, page+i, allocPhys, 0x3);
            else if (!ptable->options.present)
                ptable = page_table_create(pdir, page+i, allocPhys, 0x3);
            else if (!ptable->options.dir_location)
                ptable = page_table_create(pdir, page+i, allocPhys, 0x3);
            else
                ptable->options.dir_location = (allocPhys >> 12);

            if (i >= 0)
                ptable->options.chained_prev = 1;

            if (i < (count-1)) {
                ptable->options.chained_next = 1;
            }

            if (vmm_area == VMM_AREA_USER)
                ptable->options.user = 1;

            //knotice("page %d\\%d: 0x%x 0x%x paddr 0x%x", page, i, ptable, ptable->addr,
            //allocPhys);

            if (page+i > MAX_TABLE_COUNT) {
                page = 0;
                i = 0;
                count -= i;

                dir++;
                pdir = page_dir_get(dir);

                if (!pdir->options.present)
                    pdir = page_dir_create(dir, 0x3);

                if (vmm_area == VMM_AREA_USER)
                    pdir->options.user = 1;

            }

            allocPhys += VMM_PAGE_SIZE;
        }

        if (max_virt[vmm_area] < addr)
          max_virt[vmm_area] += (count * VMM_PAGE_SIZE);

        knotice("VMM: Allocated %d pages at virtaddr 0x%x, "
            "mapped to physaddr 0x%x", count, addr, phys);
        return addr;
    }


/*  Map 'count' pages to 'addr' and return 'addr' or 0 on fail */
virtaddr_t vmm_map_page(virtaddr_t addr, unsigned int vmm_area, size_t count)
{
    unsigned dir = (addr >> 22) & 0x3ff;
    unsigned page = (addr >> 12) & 0x3ff;
    //knotice("actual dir = %x, %x (%x)", dir, page, addr);

    if (!vmm_check_if_page_allocated(&dir, &page, count)) {
        return NULL; // No more pages.
    }

    addr = ((dir << 22) | (page << 12));

    pdir_t* pdir = page_dir_get(dir);

    if (!pdir)
        pdir = page_dir_create(dir, 0x3);
    else if (!pdir->addr)
        pdir = page_dir_create(dir, 0x3);
    else if (!pdir->options.present)
        pdir = page_dir_create(dir, 0x3);
    else if (!pdir->options.dir_location)
        pdir = page_dir_create(dir, 0x3);

    if (vmm_area == VMM_AREA_USER)
        pdir->options.user = 1;

    //knotice("pdir %d at 0x%x, addr 0x%x", dir, pdir,
    //    pdir->addr);

    /* Create the pages */
    for (size_t i = 0; i < count; i++) {
        ptable_t* ptable = page_table_get(pdir, page+i);

        if (!ptable)
            ptable = page_table_create(pdir, page+i,
                pmm_alloc(1, PMM_REG_DEFAULT), 0x3);
        else if (!ptable->options.present)
            ptable = page_table_create(pdir, page+i,
                pmm_alloc(1, PMM_REG_DEFAULT), 0x3);
        else if (!ptable->options.dir_location)
            ptable = page_table_create(pdir, page+i,
                pmm_alloc(1, PMM_REG_DEFAULT), 0x3);
        else
            ptable->options.dir_location = (pmm_alloc(1, PMM_REG_DEFAULT) >> 12);


        if (i > 0)
            ptable->options.chained_prev = 1;

        if (i < (count-1)) {
            ptable->options.chained_next = 1;
        }

        if (vmm_area == VMM_AREA_USER)
            ptable->options.user = 1;

    //    knotice("page %d: 0x%x 0x%x", page+i, ptable, ptable->addr);

        if (page+i > MAX_TABLE_COUNT) {
            page = 0;
            i = 0;
            count -= i;

            dir++;
            pdir = page_dir_get(dir);

            if (!pdir->options.present)
                pdir = page_dir_create(dir, 0x3);

            if (vmm_area == VMM_AREA_USER)
                pdir->options.user = 1;

        }
    }

 //   max_virt[vmm_area] += (count * VMM_PAGE_SIZE);
    knotice("VMM: Allocated %d pages at virtaddr 0x%x", count, addr);
    return addr;
}
