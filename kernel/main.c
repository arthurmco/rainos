/* RainOS main kernel entry */

#include <stdint.h>

#include "arch/i386/devices/vga.h"
#include "arch/i386/devices/ioport.h"
#include "arch/i386/devices/serial.h"
#include "arch/i386/irq.h"
#include "arch/i386/idt.h"
#include "arch/i386/fault.h"
#include "terminal.h"
#include "ttys.h"

#include <kstdlib.h>
#include <kstring.h>
#include <kstdlog.h>
#include <kstdio.h>

void kernel_main(uint32_t mboot) {
    terminal_t term_stdio;
    term_stdio.defaultColor = 0x07;
    terminal_set(&term_stdio);
    vga_init(&term_stdio);

    /* Initialize logging terminal device */
    terminal_t term_log;
    term_log.defaultColor = 0x07;
    ttys_init(&term_log);
    klog_set_device(&term_log);

    knotice("IDT created");
    idt_init();

    knotice("Fault handlers created");
    fault_init();

    knotice("IRQ handlers created");
    irq_init();

    terminal_clear();
    terminal_setcolor(0x09);
    puts("Starting RainOS\n");
    terminal_setcolor(0x0f);
    puts("Copyright (C) 2016 Arthur M\n");
    terminal_restorecolor();
    puts("\tLicensed under GNU GPL 2.0\n\n");

    asm("sti");
    puts("=");
    kwarn("[jc] Wrong TAMPA at address 0xfe300000");
    kerror("[bambam] Bad TRAPEZERA at address 0x13131313");

    for(;;) {
        asm volatile("nop");
    }
}
