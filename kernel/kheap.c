#include "kheap.h"

static struct HeapList hUsed;
static struct HeapList hFree;

static kheap_addItem(heap_item_t* item, heap_item_t* prev, struct HeapList* list) {
    item->prev = prev;
    if (prev) {
        item->next = prev->next;
        prev->next = item;

        if (item->next) {
            item->next->prev = item;
        }
    }

    if (!prev) {
        list->first = item;
        if (list->count == 0) {
            list->last = item;
        }
    }

    if (!item->next) {
        list->last = item;
    }

    list->count++;
}

static kheap_removeItem(heap_item_t* item, struct HeapList* list) {
    heap_item_t* prev = item->prev;

    if (list->first == item) {
        list->first = item->next;
    }

    if (list->last == item) {
        list->last = item->prev;
    }

    if (prev) {
        prev->next = item->next;
        if (item->next) {
            item->next->prev = prev;
        }
    }


    list->count--;
}

static heap_item_t* item_reserve_bottom;
static heap_item_t* item_reserve_top;

static uintptr_t addr_reserve_bottom;
static uintptr_t addr_reserve_top;

void kheap_init()
{
    // Allocate 2 pages for the item reserve
    item_reserve_bottom = (heap_item_t*)vmm_alloc_page(VMM_AREA_KERNEL, 2);
    memset(item_reserve_bottom, 0, 2*VMM_PAGE_SIZE);

    item_reserve_top = item_reserve_bottom;

    // Allocate 16 pages for the data reserve
    addr_reserve_bottom = vmm_alloc_page(VMM_AREA_KERNEL, 16);
    addr_reserve_top = addr_reserve_bottom;

    knotice("KHEAP: items starts at 0x%x, addresses starts at 0x%x",
        item_reserve_bottom, addr_reserve_bottom);

    hUsed.count = 0;
    hFree.count = 0;
}

virtaddr_t kheap_allocate(size_t bytes)
{
    heap_item_t* item = _kheap_alloc_item(bytes);

    if (!item->addr) {
        item->addr = addr_reserve_top;

        // Round to the next multiple of 4.
        // and reserve space to store the canary
        item->bytes = ((bytes + 3) & ~3) + sizeof(uint32_t);

        item->flags = HFLAGS_USED;
        item->canary = (item->addr) & ~bytes;
        kheap_addItem(item, hUsed.last, &hUsed);
    } else {
        item->flags = HFLAGS_USED;
        item->canary = (item->addr) & ~bytes;
        kheap_addItem(item, _kheap_find_item(&hUsed, item->addr,
            HFIND_NEAREST_BELOW), &hUsed);
    }

    addr_reserve_top += item->bytes;
    return item->addr;
}

virtaddr_t kheap_allocate_phys(physaddr_t phys, size_t bytes)
{
    heap_item_t* item = _kheap_alloc_item(bytes);

    if (!item->addr) {
        item->addr = addr_reserve_top;

        // Round to the next multiple of 4.
        // and reserve space to store the canary
        item->bytes = ((bytes + 3) & ~3) + sizeof(uint32_t);

        item->flags = HFLAGS_USED | HFLAGS_PHYS | (phys & ~0xfff);
        item->canary = (item->addr) & ~bytes;
        kheap_addItem(item, _kheap_find_item(&hUsed, item->addr,
            HFIND_NEAREST_BELOW), &hUsed);
    } else {
        item->flags = HFLAGS_USED;
        item->canary = (item->addr) & ~bytes;
        kheap_addItem(item,
            _kheap_find_item(&hUsed, item->addr,
                HFIND_NEAREST_BELOW), &hUsed);
    }

    addr_reserve_top += item->bytes;
    return item->addr;
}
void kheap_deallocate(virtaddr_t addr)
{
    /* Remove this from the used list and put them on the free list */
    heap_item_t* item = _kheap_find_item(&hUsed, addr, NULL);
    knotice("KHEAP: called 0x%x, found 0x%x",
        addr, (item) ? item->addr : -1);

    if (!item)
        return; //No item there

    item->flags = HFLAGS_FREE;
    kheap_removeItem(item, &hUsed);
    kheap_addItem(item, _kheap_find_item(&hFree, addr, HFIND_NEAREST_BELOW),
        &hFree);
}

/* Allocate an item.
    This can mean create a new item or finding an existant item */
heap_item_t* _kheap_alloc_item(size_t bytes)
{
    if (hFree.count == 0) {
        return item_reserve_top++;
    }


    heap_item_t* it = hFree.first;
    /* we need to get bytes rounded up to multiple of 4 plus canary space */
    size_t real_bytes =  ((bytes + 3) & ~3) + sizeof(uint32_t);

    for (int i = 1; i < hFree.count; i++) {

        if (!it) {
            continue;
        }

        if (it->flags != HFLAGS_FREE) {
            it = it->next;
            continue;
        }

        knotice("alloc %d (%d) bytes, trial %d, found %d at 0x%x",
            bytes, real_bytes, i, it->bytes, it->addr);
        retry_detect:
        if (it->bytes == real_bytes) {
            knotice("got 0x%x", it->addr);
            kheap_removeItem(it, &hFree);
            return it;
        }

        if (it->bytes > real_bytes) {
            size_t alloc = it->bytes;
            size_t divide = (it->bytes) / real_bytes;


            if (divide < 2)
                divide = 2;

            /* Change them */
            knotice("dividing item of %d bytes", it->bytes);
            it->bytes /= divide;
            knotice("now item has %d bytes", it->bytes);

            heap_item_t* previt = it;
            for (int i = 0; i < divide; i++) {
                /* Split them into smaller items */
                heap_item_t* newit = item_reserve_top++;
                newit->bytes = it->bytes / divide;
                newit->addr = (it->addr) + (i*it->bytes);
                newit->flags = HFLAGS_FREE;
                kheap_addItem(newit, previt, &hFree);
                knotice("complimented, addr 0x%x, bytes %d",
                    newit->addr, newit->bytes);
                previt = newit;
                goto retry_detect; /* Retry, to see if we can allocate now */

            }
        }

        if (it->bytes < real_bytes) {
            /* Join smaller free pieces */
            int complete = 0;
            knotice("joining pieces");

            heap_item_t* nextit = it->next;
            while (!complete) {

                knotice("piece of %d at 0x%x", it->bytes, it->addr);

                if (!nextit) {
                    break;
                }

                if (nextit->flags != HFLAGS_FREE) {
                    break;
                }

                /* if free and valid, join both into one */
                it->bytes += nextit->bytes;

                knotice("now has %d bytes", it->bytes);
                /* TODO: put the now useless entry into a list,
                    to be reallocated later */

                /* remove the now useless item */
                kheap_removeItem(nextit, &hFree);

                if (it->bytes >= real_bytes) {
                    complete = 1;
                } else {
                    nextit = it->next;
                }
            }

            if (complete) {
                goto retry_detect; //see if we can get some address now.
            }

        }

        it = it->next;
    }

    /* Last resort */
    return item_reserve_top++;

}

#define HEAP_LIST_WALK(item, count) \
    for (unsigned i = 0; i < count; i++) item = item->next;

/* Find an item that represents the address 'addr'
    if mode & HFIND_NEAREST_ABOVE, return the nearest item bigger than the address
    if mode & HFIND_NEAREST_BELOW, return the nearest item smaller than the address */
heap_item_t* _kheap_find_item(struct HeapList* const list, virtaddr_t addr, int mode)
{
    heap_item_t* start = list->first;
    heap_item_t* end = list->last;
    heap_item_t* half = start;

    if (list->count == 0) {
        /* No items, no shit */
        return NULL;
    }

    if (addr < start->addr && (mode & HFIND_NEAREST_ABOVE)) {
        return start;
    }

    if (addr > end->addr && (mode & HFIND_NEAREST_BELOW)) {
        return end;
    }

    size_t step = list->count/2;

    while (step > 3) {
        HEAP_LIST_WALK(half, step);
        step /= 2;

        /* Detect exactly equal addresses */
        if (start->addr == addr && !mode) {
            /* Is at start */
            return start;
        } else if (half->addr == addr && !mode) {
            /* Is at half */
            return half;
        } else if (end->addr == addr && !mode) {
            /* Is at end */
            return end;
        }

        /* Split the memory */
        if (addr > start->addr && addr < half->addr) {
            /* Address is between start and half */
            end = half;
            HEAP_LIST_WALK(half, step);
        } else if (addr > half->addr && addr < end->addr) {
            /* Address is between half and end */
            start = half;
            HEAP_LIST_WALK(half, step);
        }

    }

    /* Now we have only start, end and half */
    /* Detect equal addresses */
    if (start->addr == addr && !mode) {
        /* Is at start */
        return start;
    } else if (half->addr == addr && !mode) {
        /* Is at half */
        return half;
    } else if (end->addr == addr && !mode) {
        /* Is at end */
        return end;
    }

    /* Detect smaller addresses */
    if (addr < start->addr && (mode & HFIND_NEAREST_ABOVE)) {
        return start;
    } else if (addr < half->addr && addr > start->addr &&
        (mode & HFIND_NEAREST_ABOVE)) {
        return half;
    } else if (addr < end->addr && addr > half->addr &&
        (mode & HFIND_NEAREST_ABOVE)) {
        return end;
    }

    /* Detect bigger addresses */
    if (addr > start->addr && addr < half->addr &&
        (mode & HFIND_NEAREST_BELOW)) {
        return start;
    } else if (addr > half->addr && addr < end->addr &&
        (mode & HFIND_NEAREST_BELOW)) {
        return half;
    } else if (addr > end->addr && (mode & HFIND_NEAREST_BELOW)) {
        return end;
    }

    /* Well... */
    return NULL;
}
