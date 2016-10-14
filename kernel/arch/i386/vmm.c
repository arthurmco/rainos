#include "vmm.h"

/* Stores the maximum address allocated for these areas */
static struct virt_region_t areas[VMM_AREA_COUNT];


void vmm_init(uintptr_t kernel_start, uintptr_t kernel_end,
    uintptr_t kernel_virt_offset)
    {
        /* TODO: remove this when you go to higher half */
        kernel_virt_offset = 0xc0000000;

        areas[VMM_AREA_USER].min_addr = 0;
        areas[VMM_AREA_USER].max_addr = kernel_virt_offset-1;
        areas[VMM_AREA_USER].first_free_addr = (kernel_end + 0x1000) & ~0xfff;

        areas[VMM_AREA_KERNEL].min_addr = kernel_virt_offset;
        areas[VMM_AREA_KERNEL].max_addr = 0xffffffff;
        areas[VMM_AREA_KERNEL].first_free_addr = kernel_end + kernel_virt_offset;

    }

/*  Check if some pages are allocated.
    Outputs next non-allocated dir and table */
static int vmm_check_if_page_allocated(unsigned int dir, unsigned int table,
    size_t pages, int area)
    {
        size_t p = 0;

        for (unsigned d = dir; d < 1024; d++) {
            pdir_t* pd = page_dir_get(d);

            /* If pdir not exist, create it */
            if (!pd || !pd->options.present) {
                pd = page_dir_create(d, 0x3); //present and writeable
            }

            //knotice("pdir for %d - %x (%x)", dir, pd, pd->addr);

            if (area == VMM_AREA_USER) {
                pd->options.user = 1;
            }

            for (unsigned t = table; t < 1024; t++) {
                ptable_t* pt = page_table_get(pd, t);
                //knotice("ptable of %x is %x for %d", pd, pt, t);

                /* If table doesn't exist, create it */
                if (!pt || !pt->options.present) {
                    pt = page_table_create(pd, t,
                        0, 0x3);
                }

                if (area == VMM_AREA_USER) {
                    pt->options.user = 1;
                }
                p++;

                //knotice("#%d: %x (at 0x%x)", p, pt->addr, pt);

                if (p == pages) {
                    return 1;
                }

            }

            table = 0;
        }

        return 0;
    }

/* Allocate 'count' pages from virtual space*/
virtaddr_t vmm_alloc_page(unsigned int vmm_area, size_t count)
{
    knotice("VMM: Allocating %d pages from vmm area %d", count, vmm_area);
    /*  Same as vmm_alloc_physical, but without need of getting the physical
        addr back */
    physaddr_t p;
    virtaddr_t v = vmm_alloc_physical(vmm_area, &p, count, PMM_REG_DEFAULT);
    areas[vmm_area].first_free_addr = v + (count * VMM_PAGE_SIZE);
    knotice("VMM: Allocated %x -> %x", v, p);
    return v;
}

/*  Deallocate pages .
    This usually means set the present bit to 0 */
void vmm_dealloc_page(virtaddr_t addr, size_t count)
{
    unsigned int pdir, ptbl;
    pdir = GET_DIR(addr);
    ptbl = GET_TABLE(addr);

    size_t page = 0;
    for (unsigned int d = pdir; d < 1024; d++) {
        pdir_t* pd = page_dir_get(d);
        for (unsigned int t = ptbl; t < 1024; t++) {
            ptable_t* pt = page_table_get(pd, t);

            if (pt->options.present) {
                pt->options.present = 0;
            } else {
                kerror("VMM: tried to free an already-free page at "
                    "dir %x tbl %x (addr 0x%x) [%x]", pdir, ptbl, addr,
                    *((unsigned int*)pt));
            }

            page++;
            if (page == count) return;
        }
        ptbl = 0;
    }
}

/*  Alloc 'count' pages and store their physaddr on
    the 'phys' variable */
virtaddr_t vmm_alloc_physical(unsigned int vmm_area,
    physaddr_t* phys, size_t count, int pmm_type)
    {
        virtaddr_t addr = areas[vmm_area].first_free_addr;

        unsigned int pdir, ptbl;
        pdir = GET_DIR(addr);
        ptbl = GET_TABLE(addr);

        knotice("Allocating at pdir %d ptbl %d", pdir, ptbl);

        /* Tries to allocate until we cross the area boundary */
        while (!vmm_check_if_page_allocated(pdir, ptbl, count, vmm_area)) {
            ptbl += count;

            if (ptbl > 1023) {
                ptbl = (ptbl - 1023);
                pdir++;

                if (GET_DIR(areas[vmm_area].max_addr) <= pdir) {
                    panic("VMM: Out of memory in %s virtual memory region!",
                        vmm_area == VMM_AREA_USER ? "user" : "kernel");
                    return NULL; /* Out of area */
                }
            }

        }

        *phys = 0;
        physaddr_t ph = pmm_alloc(count, pmm_type);

        knotice("Allocated at directory/table %x:%x to physaddr 0x%x",
            pdir, ptbl, ph);

        size_t page = 0;
        for (unsigned int d = pdir; d < 1024; d++) {
            pdir_t* pd = page_dir_get(d);
            for (unsigned int t = ptbl; t < 1024; t++) {
                ptable_t* pt = page_table_get(pd, t);
                knotice("--> %d of %d", page, count);

                /* Fills the memory physical addresses */
                pt->options.dir_location = ((ph >> 12) + page);

                if (vmm_area == VMM_AREA_USER)
                    pt->options.user = 1;

                page++;

                if (page == count) {
                    pt->options.chained_next = 0;
                    *phys = ph;
                    virtaddr_t naddr = (pdir << 22) | (ptbl << 12);
                    virtaddr_t nfree = (naddr) + (count * 4096);

                    if (areas[vmm_area].first_free_addr < nfree) {
                        areas[vmm_area].first_free_addr = nfree;
                    }

                    knotice("VMM: allocated address 0x%x, first free is %x",
                        naddr, nfree);
                    return naddr;
                }
            }
            ptbl = 0;
        }



    }

/*  Map 'count' pages and map these pages to physical address
        'phys' . You need to allocate these address first!*/
virtaddr_t vmm_map_physical(unsigned int vmm_area,
    physaddr_t phys, size_t count)
    {
        virtaddr_t addr = areas[vmm_area].first_free_addr;

        unsigned int pdir, ptbl;
        pdir = GET_DIR(addr);
        ptbl = GET_TABLE(addr);

        /* Tries to allocate until we cross the area boundary */
        while (!vmm_check_if_page_allocated(pdir, ptbl, count, vmm_area)) {
            ptbl += count+1;

            if (ptbl > 1023) {
                ptbl = (ptbl - 1023);
                pdir++;

                if (GET_DIR(areas[vmm_area].max_addr) >= pdir) {
                    panic("VMM: Out of memory in %s virtual memory region!",
                        vmm_area == VMM_AREA_USER ? "user" : "kernel");
                    return NULL; /* Out of area */
                }
            }

        }


        knotice("Allocated at directory/table %x:%x to physaddr 0x%x",
            pdir, ptbl, phys);

        physaddr_t ph = phys;

        size_t page = 0;
        for (unsigned int d = pdir; d < 1024; d++) {
            pdir_t* pd = page_dir_get(d);
            for (unsigned int t = ptbl; t < 1024; t++) {
                ptable_t* pt = page_table_get(pd, t);
                knotice("---> %d of %d", page, count);

                /* Fills the memory physical addresses */
                pt->options.dir_location = ((ph >> 12) + page);

                if (vmm_area == VMM_AREA_USER) {
                    pd->options.user = 1;
                    pt->options.user = 1;
                }

                pt->options.chained_next = 1;
                if (page > 0)
                    pt->options.chained_prev = 1;


                page++;

                if (page == count) {
                    pt->options.chained_next = 0;
                    return areas[vmm_area].first_free_addr;
                }
            }
            ptbl = 0;
        }

    }

/*  Map 'count' pages to 'addr' and return 'addr' or 0 on fail */
virtaddr_t vmm_map_page(virtaddr_t addr, unsigned int vmm_area, size_t count)
{
    unsigned int pdir, ptbl;
    pdir = GET_DIR(addr);
    ptbl = GET_TABLE(addr);
    if (!vmm_check_if_page_allocated(pdir, ptbl, count, vmm_area)) {
        return NULL;
    }

    pdir_t* pd = page_dir_get(pdir);
    ptable_t* pt = page_table_get(pd, ptbl);

    if (!pt) {
        pt = page_table_create(pd, ptbl,
            pmm_alloc(1, PMM_REG_DEFAULT), 0x3);
    }

    /* Allocate physical addresses */
    for (int i = 0; i < count; i++) {
        physaddr_t ph = pmm_alloc(1, PMM_REG_DEFAULT);

        ptable_t* pt = page_table_get(pd, ptbl+i);
        if (!pt) {
            pt = page_table_create(pd, ptbl+i,
                ph, 0x3);
        }

        if (vmm_area = VMM_AREA_USER)
            pt[i].options.user = 1;
    }

    return addr;
}
