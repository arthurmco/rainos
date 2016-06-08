#include "kheap.h"

static struct HeapList hUsed;
static struct HeapList hFree;

static kheap_addItem(heap_item_t* item, heap_item_t* prev, struct HeapList* list) {
    item->prev = prev;
    if (prev) {
        item->next = prev->next;
        item->next->prev = item;
    }

    if (!prev) {
        list->first = item;
    }

    if (!item->next) {
        list->last = item;
    }

    list->count++;
}

static kheap_removeItem(heap_item_t* item, struct HeapList* list) {
    heap_item_t* prev = item->prev;

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
        kheap_addItem(item, item->prev, &hUsed);
    } else {
        item->flags = HFLAGS_USED;
        item->canary = (item->addr) & ~bytes;
        kheap_addItem(item, item->prev, &hUsed);
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
        kheap_addItem(item, item->prev, &hUsed);
    } else {
        item->flags = HFLAGS_USED;
        item->canary = (item->addr) & ~bytes;
        kheap_addItem(item, item->prev, &hUsed);
    }

    addr_reserve_top += item->bytes;
    return item->addr;
}
void kheap_deallocate(virtaddr_t addr)
{

}

/* Allocate an item.
    This can mean create a new item or finding an existant item */
heap_item_t* _kheap_alloc_item(size_t bytes)
{
    return item_reserve_top++;
}

/* Find an item that represents the address 'addr'
    if mode & HFIND_NEAREST_ABOVE, return the nearest item bigger than the address
    if mode & HFIND_NEAREST_BELOW, return the nearest item smaller than the address */
heap_item_t* _kheap_find_item(virtaddr_t addr, int mode)
{

}
