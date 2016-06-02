#include "pmm.h"

static struct MMAPRegion* regs_data;
static int regs_count;

/*  Initialize physical memory manager.
    Autodetect RAM regions, report usable physical RAM at end
*/
int pmm_init(struct mmap_list* mm_list, physaddr_t kstart,
    /*out*/ physaddr_t* kend)
{

    regs_count = mm_list->size;

    /* No regions detected */
    if (regs_count <= 0) {
        kerror("PMM: No regions detected!");
        return 0;
    }

    /* Allocate memory for mmap regions, set kernel end accordingly */
    regs_data = (struct MMAPRegion*)*kend;
    *kend = (*kend) + (sizeof(struct MMAPRegion) * regs_count);

    int ir = 0;
    uint64_t freemem = 0;
    struct MMAPRegion* reg_kernel = NULL;
    for (int i = 0; i < mm_list->size; i++) {
        regs_data[ir].start = mm_list->maps[i].start;
        regs_data[ir].len = mm_list->maps[i].length;
        regs_data[ir].first_free_addr = regs_data[i].start;

        switch (mm_list->maps[i].type) {
            case MMAP_FREE: regs_data[ir].region_type = PMM_REG_DEFAULT; break;
            default: regs_data[ir].region_type = PMM_REG_HARDWARE; break;
        }

        /* if start and start + len < 1M, then map to legacy */
        if (regs_data[ir].start < 1048576 &&
            (regs_data[ir].start + regs_data[ir].len) < 1048576 &&
            regs_data[ir].region_type == PMM_REG_DEFAULT) {
                regs_data[ir].region_type = PMM_REG_LEGACY;

        }

        if (regs_data[ir].region_type == PMM_REG_LEGACY ||
            regs_data[ir].region_type == PMM_REG_DEFAULT) {
            freemem += regs_data[ir].len;
        }

        /* TODO: Treat overlapping areas */

        knotice("PMM: detected RAM type %x starting at %x, %d bytes",
            regs_data[ir].region_type, regs_data[ir].start,
            regs_data[ir].len);

        /* Get area that kernel occups */
        if (regs_data[ir].start >= kstart &&
            *kend <= regs_data[ir].start+regs_data[ir].len &&
            regs_data[ir].region_type == PMM_REG_DEFAULT ) {
                reg_kernel = &regs_data[ir];
        }

        ir++;
    }
    regs_count = ir;

    if (!reg_kernel) {
        /* Could not find kernel area */
        kerror("PMM: memory area not found for kernel (0x%x-0x%x)",
            kstart, *kend);
        return 0;
    }

    knotice("PMM: %d kB of usable RAM detected",
        (uint32_t)((freemem / 1024) & 0xffffffff));

    /* Allocate memory for bitsets */
    for (int i = 0; i < regs_count; i++) {
        regs_data[i].region_bitset = (uint8_t*)*kend;
        size_t len = 1+(regs_data[i].len / PMM_PAGE_SIZE / 8);

        memset(regs_data[i].region_bitset, 0, len);

        /*  Each bit means a page (now its 4 kB)
            C only allocates bytes, then we divide by 8 to get the amount of
            bytes for that bitset */

        *kend = (*kend) + len;

        /* Pad the bitset to 4 bytes */
        *kend = (*kend+2) & ~3;
    }

    uint64_t memocc = 0;

    /* Fill memory used by bitsets */
    uintptr_t kmemstart = kstart - reg_kernel->start;
    uintptr_t kmemend = (*kend) - reg_kernel->start;
    for (unsigned kpage = (kmemstart / PMM_PAGE_SIZE);
        kpage <= (kmemend / PMM_PAGE_SIZE);
        kpage++) {

            if (kpage + 8 < (kmemend / PMM_PAGE_SIZE)) {
                reg_kernel->region_bitset[kpage/8] = 0xff;
                kpage += 7;
                memocc += PMM_PAGE_SIZE*8;
                continue;
            }

            BITSET_SET(reg_kernel->region_bitset, kpage/8, kpage%8);
            memocc += PMM_PAGE_SIZE;

    }

    reg_kernel->first_free_addr = kstart + memocc;
    knotice("PMM: %d kB of occupied RAM",
        (uint32_t)(memocc / 1024) & 0xffffffff);
    return 1;
}

/* Get memory usage from region */
size_t pmm_get_mem_total(int region)
{

}

size_t pmm_get_mem_used(int region)
{

}

size_t pmm_get_mem_free(int region)
{

}

static physaddr_t _pmm_set_addr(physaddr_t addr, size_t pages, struct MMAPRegion* reg,
    int force_addr) {
    uint32_t startpage = (addr - reg->start) / PMM_PAGE_SIZE;
    uint32_t limit = startpage + (reg->len / PMM_PAGE_SIZE);
//    knotice("<<%x>>",addr);

    while (startpage < limit) {
        if (BITSET_ISSET(reg->region_bitset, startpage/8, startpage%8)) {
            //Occupied
            if (force_addr)
                return NULL; //Can't allocate at exact address.

//            knotice("fuck %d", startpage);
            startpage++;
        } else {
            //Not occupied.
            //Check if all future pages are occupied.
            size_t page = 0;
            do {
                if (BITSET_ISSET(reg->region_bitset,
                        (startpage+page)/8, (startpage+page)%8)) {
                    //One of future pages are occupied.
                    //Reset the outer loop and find some other occupied.
                    startpage += page;
//                    knotice("shit %d", startpage);
                }
                page++;
            } while (page < pages);

            if (page == pages) {
                break;
            } else {
                if (force_addr)
                    return NULL; //Can't allocate at exact address.
            }
        }

    }

    addr = reg->start + (startpage * PMM_PAGE_SIZE);

    /* all OK, fill the buffer */
    for (size_t p = 0; p < pages; p++) {
//        knotice("%x", addr+(p*PMM_PAGE_SIZE));
        BITSET_SET(reg->region_bitset, (startpage+p)/8, (startpage+p)%8);
    }

    if (!force_addr)
        reg->first_free_addr = addr + (pages * PMM_PAGE_SIZE);

    return addr;
}

static physaddr_t _pmm_unset_addr(physaddr_t addr, size_t pages, struct MMAPRegion* reg) {
    uint32_t startpage = (addr - reg->start) / PMM_PAGE_SIZE;
    uint32_t limit = startpage + (reg->len / PMM_PAGE_SIZE);

    knotice("<<%x>> <<%x>> %d", addr, limit, pages);
    if ((startpage + pages) > limit)
        return NULL; //Unset past end of the buffer.

    /* unset the buffer */
    for (size_t p = 0; p < pages; p++) {
        knotice("%x", addr+(p*PMM_PAGE_SIZE));
        knotice("[[%x]]", reg->region_bitset[(startpage+p)/8] );
        BITSET_UNSET(reg->region_bitset, (startpage+p)/8, (startpage+p)%8);
    }


    reg->first_free_addr = addr;

    return addr;
}


physaddr_t pmm_alloc(size_t pages, int type)
{
    struct MMAPRegion* reg = NULL;
    for (int i = 0; i < regs_count; i++) {
        if (regs_data[i].region_type == type) {
            if (regs_data[i].first_free_addr+(pages*PMM_PAGE_SIZE) >
                regs_data[i].start + regs_data[i].len) {
                    /* Memory request doesn't fit on this region */
                    continue;
            }
            reg = &regs_data[i];
            break;
        }
    }

    /* Could not find suitable region */
    if (!reg) {
        kerror("Could not allocate memory!");
        return NULL;
    }

    physaddr_t addr = reg->first_free_addr;
    return _pmm_set_addr(addr, pages, reg, 0);
}

physaddr_t pmm_reserve(physaddr_t addr, size_t pages)
{
    struct MMAPRegion* reg = NULL;
    for (int i = 0; i < regs_count; i++) {
        if (regs_data[i].region_type == PMM_REG_FAULTY) {
            return NULL; //Can't reserve faulty RAM.
        }

        if (addr >= regs_data[i].start &&
            addr < regs_data[i].start + regs_data[i].len ) {

            if (regs_data[i].first_free_addr + (pages * PMM_PAGE_SIZE) >
                regs_data[i].start + regs_data[i].len) {
                    continue; //Region doesn't fit.
            }

            reg = &regs_data[i];
            break;
        }

    }

    if (!reg) {
        kerror("Could not reserve memory at 0x%x", addr);
        return NULL;
    }
    return _pmm_set_addr(addr, pages, reg, 1);
}

int pmm_free(physaddr_t addr, size_t pages)
{
    struct MMAPRegion* reg = NULL;
    for (int i = 0; i < regs_count; i++) {
        if (regs_data[i].region_type == PMM_REG_FAULTY) {
            return NULL; //Can't reserve faulty RAM.
        }

        if (addr >= regs_data[i].start &&
            addr < regs_data[i].start + regs_data[i].len ) {

            if (regs_data[i].first_free_addr + (pages * PMM_PAGE_SIZE) >
                regs_data[i].start + regs_data[i].len) {
                    continue; //Region doesn't fit.
            }

            reg = &regs_data[i];
            break;
        }

    }

    if (!reg)
        return NULL; //Can't find region

    return (_pmm_unset_addr(addr, pages, reg) > 0x0);

}
