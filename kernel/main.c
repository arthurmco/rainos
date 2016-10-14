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
#include "arch/i386/devices/ps2_kbd.h"
#include "arch/i386/devices/pci.h"
#include "arch/i386/devices/floppy.h"
#include "arch/i386/devices/ata.h"
#include "arch/i386/multiboot.h"
#include "arch/i386/vmm.h"
#include "arch/i386/tss.h"
#include "arch/i386/irq.h"
#include "arch/i386/idt.h"
#include "arch/i386/pages.h"
#include "arch/i386/fault.h"
#include "terminal.h"
#include "time.h"
#include "ttys.h"
#include "mmap.h"
#include "pmm.h"
#include "kheap.h"
#include "initrd.h"
#include "keyboard.h"
#include "kshell.h"
#include "elf.h"
#include "vfs/vfs.h"
#include "vfs/partition.h"
#include "vfs/fat.h"
#include "vfs/sfs.h"

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
    static terminal_t term_stdio;
    term_stdio.defaultColor = 0x07;
    terminal_set(&term_stdio);
    vga_init(&term_stdio);
    //ttys_init(&term_stdio);

    /* Initialize logging terminal device */
    static terminal_t term_log;
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

    static const char* map_types[] = {"Unknown", "Free RAM", "Reserved RAM", "\0"};
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

    multiboot_mod_t* mod = (multiboot_mod_t*)mboot->modules_addr;
    /* Get modules */

    multiboot_mod_t* initrd = NULL;

    for (int i = 0; i < mboot->modules_count; i++) {
        const char* mname = mod->string_ptr ?
            (const char*)(mod->string_ptr) : "<NULL>";
        knotice("KERNEL: discovered mboot module '%s' "
                "[start: 0x%08x, end: 0x%08x]", mname,
                mod->mod_start, mod->mod_end);
        kend = (kend > mod->mod_end) ? kend : mod->mod_end;

        if (!strncmp("initrd", mname, 6))
            initrd = mod;

        mod++;
    }

    if (!initrd) {
        kerror("Couldn't find initrd.rain");
        panic("FATAL: initrd not found! Can't continue.");
    }


    /* Page-align the address */
    kend = (kend + 0x1000) & ~0xfff;
    knotice("KERNEL: new kend is at 0x%08x", kend);
    //Init physical memory manager
    WRITE_STATUS("Starting physical memory manager...");
    if (!pmm_init(&mml, kstart, &kend)) {
        WRITE_FAIL();
        kprintf("PANIC: error while starting memory manager");
        _cli();
        return;
    }

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
    vmm_init(kstart, kend, page_dir_phys);

    WRITE_SUCCESS();

    WRITE_STATUS("Starting kernel heap...");
    kheap_init();

    WRITE_SUCCESS();

    WRITE_STATUS("Starting devices...\t ");
    kprintf(" \n  pit");
    pit_init();
    kprintf("\tok!");

    // Start serial device in the right way
    kprintf(" \n  serial");
    serial_init();
    kprintf("\tok!");

    kprintf(" \n  floppy");
    if (floppy_init())
        kprintf("\tok!");
    else
        kprintf("\tfail!");


    kprintf(" \n  pci");
    pci_init();
    kprintf("\tok!");


    if (!term_stdio.term_getc) {
    	kprintf(" \n keyboard");
    	if (!i8042_init()) {
            kprintf("\t fail!");
    	} else {
            kbd_init();
            kbd_init();
            keyboard_init(&term_stdio);
    		kprintf("\t ok!");
    	}
    }


    size_t pci_count = pci_get_device_count();
    for (size_t i = 0; i < pci_count; i++) {
        struct PciDevice* dev = pci_get_device_index(i);

        if (ata_check_device(dev)) {
            kprintf("\n  ata");
            if (ata_initialize(dev)) {
                kprintf("\tok!");
            }
        }
    }

    WRITE_SUCCESS();

    WRITE_STATUS("Starting VFS...");
    vfs_init();
    partitions_init();

    //init filesystems
    fat_init();

    WRITE_SUCCESS();

    if (initrd) {
        WRITE_STATUS("Starting initrd");
        if (!initrd_init(initrd->mod_start, initrd->mod_end)) {
            WRITE_FAIL();
            goto next3;
        }

        if (!initrd_mount()) {
            kprintf("\n");
            kerror("Failed to mount initrd");
            WRITE_FAIL();
            goto next3;
        }

        WRITE_SUCCESS();
    }

    next3:
//        kprintf("\n\n Nothing to do. Starting kernel shell... \n");

    WRITE_STATUS("Jumping to user mode (will call init on the future)");

    tss_init(page_dir_phys);

    uint8_t* newfunc = (uint8_t*)vmm_alloc_page(VMM_AREA_USER, 3);
    void* newstack = (vmm_alloc_page(VMM_AREA_USER, 1) + (VMM_PAGE_SIZE)-16);
    knotice("opening bintest.bin at %x", newfunc);

    vfs_node_t* node_file = vfs_find_node("/dir/bintest.bin");

    if (!node_file) {
        knotice("not found");
        WRITE_FAIL();
        _halt();
    }

    size_t read = vfs_read(node_file, 0, -1, (void*)newfunc);
    kprintf("\n%s - %d bytes\n", node_file->name, read);
    kprintf("\n");
    for (size_t i = 0; i <= (read/16); i++) {
        for (size_t j = 0; j < 16; j++) {
            kprintf("%02x ", newfunc[i*16+j]);
        }

        kprintf(" | ");

        for (size_t j = 0; j < 16; j++) {
            char c =  (char)newfunc[i*16+j];

            if (c < 32) {
                putc('.');
            } else {
                putc(c);
            }
        }

        if (i % 8 == 0 && i > 0) {
            kgetc();
        }

        kprintf("\n");
    }

    char buf[64];
    vfs_get_full_path(node_file, buf);
    kprintf("File: %s <0x%x 0x%x>\n\n", buf, newfunc, newstack);
    //jump_usermode((uintptr_t)newstack, (uintptr_t)newfunc);

    vfs_node_t* elfnode = vfs_find_node("/dir/bintest.elf");
    if (!elfnode) {
        kerror("Our test ELF file wasn't found");
    }
    vfs_get_full_path(elfnode, buf);
    kprintf("Test ELF file: %s\n", buf);

    elf_exec_t* elf_exec = elf_open_file(elfnode);
    if (elf_exec) {
        int s = elf_parse_sections(elf_exec);
        int p = elf_parse_phs(elf_exec);
        kprintf("\t %d sections and %d program segments found", s, p);
        //elf_execute_file(elf_exec);
    }

    WRITE_FAIL();
    /* for(;;) {
        terminal_setcolor(0x0f);
        terminal_puts("kernsh> ");
        terminal_restorecolor();
        char s[128];
        kgets(s, 128);

        kprintf("You typed: %s", s);

    } */

    struct time_tm t;
    t.second = 0;
    t.minute = 0;
    t.hour = 0;
    t.day = 0;
    t.month = 0;
    t.year = 0;
    time_init(t);

    kshell_init();
}
