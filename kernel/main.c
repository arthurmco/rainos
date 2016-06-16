/* RainOS main kernel entry */

#include <stdint.h>

#include <kstdlib.h>
#include <kstring.h>
#include <kstdlog.h>
#include <kstdio.h>

#include "arch/i386/devices/vga.h"
#include "arch/i386/devices/ioport.h"
#include "arch/i386/devices/serial.h"
#include "arch/i386/devices/pit.h"
#include "arch/i386/devices/pci.h"
#include "arch/i386/devices/ata.h"
#include "arch/i386/multiboot.h"
#include "arch/i386/vmm.h"
#include "arch/i386/tss.h"
#include "arch/i386/irq.h"
#include "arch/i386/idt.h"
#include "arch/i386/pages.h"
#include "arch/i386/fault.h"
#include "terminal.h"
#include "ttys.h"
#include "mmap.h"
#include "pmm.h"
#include "kheap.h"
#include "vfs/vfs.h"
#include "vfs/partition.h"

volatile int _timer = 0;
void timer(regs_t* regs) {
    kprintf("Timer: %d\n", _timer++);
}

extern void jump_usermode(uintptr_t stack, uintptr_t usermode_function);

#define WRITE_STATUS(status) do {   \
    terminal_setcolor(0xf);         \
    kputs(" o ");                   \
    terminal_restorecolor();        \
    kputs(status);                  \
} while (0)                         \

#define WRITE_FAIL() do {           \
    terminal_setx(70);              \
    kputs("[ ");                    \
    terminal_setcolor(0xc);         \
    kputs("FAIL");                  \
    terminal_restorecolor();        \
    kputs(" ]\n");                  \
} while (0)                         \

#define WRITE_SUCCESS() do {        \
    terminal_setx(70);              \
    kputs("[ ");                    \
    terminal_setcolor(0xa);         \
    kputs(" OK ");                  \
    terminal_restorecolor();        \
    kputs(" ]\n");                  \
} while (0)                         \


extern uintptr_t _kernel_start[];
extern uintptr_t _kernel_end[];

void kernel_main(multiboot_t* mboot, uintptr_t page_dir_phys) {
    terminal_t term_stdio;
    term_stdio.defaultColor = 0x07;
    terminal_set(&term_stdio);
    vga_init(&term_stdio);

    /* Initialize logging terminal device */
    terminal_t term_log;
    term_log.defaultColor = 0x07;
    ttys_init(&term_log);
    klog_set_device(&term_log);
    ttys_puts("\033[1mRainOS\033[0m\n");

    idt_init();
    fault_init();
    irq_init();

    terminal_clear();
    terminal_setx(20);
    terminal_setcolor(0x9f);
    kputs("RainOS\n");
    terminal_setcolor(0x0f);
    terminal_setx(20);
    kputs("Copyright (C) 2016 Arthur M\n");
    terminal_restorecolor();
    terminal_setx(20);
    kputs("\tLicensed under GNU GPL 2.0\n\n");

    _sti();

    kprintf("\t Multiboot structure at 0x%x\n", mboot);
    kprintf("\t Avaliable memory: %d.%d MB\n",
        (mboot->mem_upper + mboot->mem_lower) / 1024,
        ((mboot->mem_upper + mboot->mem_lower) % 1024) / 10);

    uintptr_t kstart = (uintptr_t)_kernel_start;
    uintptr_t kend = (uintptr_t)_kernel_end;
    knotice("KERNEL: starts at 0x%x, ends at 0x%x", kstart, kend);


    //irq_add_handler(0, &timer);
    knotice("KERNEL: Memory map: ");

    struct mmap_list mml;
    mml.size = (mboot->mmap_length / sizeof(multiboot_mmap_t))+1;

    mmap_t mm[mml.size];

    const char* map_types[] = {"Unknown", "Free RAM", "Reserved RAM", "\0"};
    int mmidx = 0;
    for (multiboot_mmap_t* map = (multiboot_mmap_t*)mboot->mmap_addr;
            map < (multiboot_mmap_t*)(((uintptr_t)mboot->mmap_addr) + mboot->mmap_length);
            map++) {

        if (map->addr_high > 0) {
            continue; // We wouldn't use areas higher than 0xffffffff
        }

        if (map->len_high > 0) {
            /* Too high */
            map->len_high = 0;
            map->len_low = 0xffffffff;
        }

        if (map->addr_low < mboot->mem_lower * 1024) {
            /*  Some BIOS make a big fat free area that gets all memory,
                including BIOS and hardware areas
                We detect them and take them off */
            if (map->len_low > 0xA0000) {
                continue; // This is one of that areas.
            }
        }

        unsigned type = map->type;
        if (type >= 3)
            type = 0;

        knotice("\t 0x%x - 0x%x, %s (code %d)", map->addr_low,
            map->addr_low+map->len_low-1, map_types[type], map->type);

        mm[mmidx].start = map->addr_low;
        mm[mmidx].length = map->len_low;
        mm[mmidx].type = map->type;
        mmidx++;
    }

    mml.maps = &mm[0];

    //Init physical memory manager
    WRITE_STATUS("Starting physical memory manager...");
    if (!pmm_init(&mml, kstart, &kend)) {
        WRITE_FAIL();
        kprintf("PANIC: error while starting memory manager");
        _cli();
        return -1;
    }

/* Test for physica memory manager */
#if 0
{
    physaddr_t addr = pmm_alloc(6, PMM_REG_DEFAULT);
    kprintf("\n\t test: allocated RAM at 0x%x", addr);
    addr = pmm_alloc(8, PMM_REG_DEFAULT);
    kprintf("\n\t test: allocated RAM at 0x%x", addr);
    addr = pmm_alloc(10, PMM_REG_DEFAULT);
    kprintf("\n\t test: allocated RAM at 0x%x", addr);
    addr = pmm_alloc(12, PMM_REG_DEFAULT);
    kprintf("\n\t test: allocated RAM at 0x%x", addr);
    addr = pmm_alloc(14, PMM_REG_DEFAULT);
    kprintf("\n\t test: allocated RAM at 0x%x", addr);
    addr = pmm_alloc(16, PMM_REG_DEFAULT);
    kprintf("\n\t test: allocated RAM at 0x%x", addr);
    addr = pmm_alloc(18, PMM_REG_DEFAULT);
    kprintf("\n\t test: allocated RAM at 0x%x", addr);
    addr = pmm_alloc(20, PMM_REG_DEFAULT);
    kprintf("\n\t test: allocated RAM at 0x%x", addr);

    uint32_t* ptr = (uint32_t*)addr;
    *ptr = 0xbadb00;
    knotice("Value: 0x%x", *ptr);

    memset(ptr, 0x1, sizeof(uint32_t));
    knotice("Value: 0x%x", *ptr);

    if (!pmm_free(addr, 20)) {
        kprintf("\n\t test: error, could not free");
    } else {
        kprintf("\n\t test: deallocated RAM at 0x%x", addr);
    }

    addr = pmm_reserve(addr, 2);
    if (addr)
        kprintf("\n\t test: ok (%x)", addr);
    else
        kprintf("\n\t test: failed, this should succeed now.");
}
#endif
    physaddr_t addr = pmm_alloc(6, PMM_REG_DEFAULT);
    if (!addr) {
        WRITE_FAIL();
        kerror("FATAL: initial physical memory allocation failed");
        _cli();
        return;
    }

    if (!pmm_free(addr, 6)) {
        WRITE_FAIL();
        kerror("FATAL: initial physical memory deallocation failed");
        _cli();
        return;
    }

    WRITE_SUCCESS();

    knotice("KERNEL: kernel end is now 0x%x", kend);

    WRITE_STATUS("Starting virtual memory manager...");
    pages_init(page_dir_phys, 0x0);
    vmm_init(kstart, &kend, page_dir_phys);

    WRITE_SUCCESS();

    WRITE_STATUS("Starting kernel heap...");
    kheap_init();

    WRITE_SUCCESS();

    WRITE_STATUS("Starting devices...\t [");
    pit_init();
    kprintf(" pit");

    pci_init();
    kprintf(" pci");

    // Start serial device in the right way
    serial_init();
    kprintf(" serial");

    size_t pci_count = pci_get_device_count();
    for (size_t i = 0; i < pci_count; i++) {
        struct PciDevice* dev = pci_get_device_index(i);

        if (ata_check_device(dev)) {
            if (ata_initialize(dev)) {
                kprintf(" ata");
            }
        }
    }

    kprintf("\t]");
    WRITE_SUCCESS();

    WRITE_STATUS("Starting VFS...");
    vfs_init();
    partitions_init();

    unsigned i = partitions_retrieve(device_get_by_name("disk0"));
    kprintf("\t%d", i);

    void* buf = kcalloc(512, 1);
    device_read(device_get_by_name("disk0p1"), 0, 512, buf);

    WRITE_SUCCESS();

#if 0

    WRITE_STATUS("Jumping to user mode (will call init on the future)");
    tss_init(page_dir_phys);

    uint8_t* newfunc = vmm_alloc_page(VMM_AREA_USER, 3);
    void* newstack = vmm_alloc_page(VMM_AREA_USER, 1);

    // Usermode code will run
    newfunc[0] = 0x66;
    newfunc[1] = 0xb8;
    newfunc[2] = 0xff;
    newfunc[3] = 0xff;   //movw $0xffff, ax
    newfunc[4] = 0xeb;
    newfunc[5] = 0xfe;   //jmp -1

    jump_usermode((uintptr_t)newstack, (uintptr_t)newfunc);


    WRITE_FAIL();
#endif
    for(;;) {
        _halt();
    }
}
