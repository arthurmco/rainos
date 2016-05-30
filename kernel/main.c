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
#include <kstdio.h>

void kernel_main(uint32_t mboot) {
    terminal_t term_stdio;
    term_stdio.defaultColor = 0x07;
    terminal_set(&term_stdio);
    ttys_init(&term_stdio);

    idt_init();
    fault_init();
    irq_init();

    terminal_clear();
    terminal_setcolor(0x09);
    puts("Starting RainOS\n");
    terminal_setcolor(0x0f);
    puts("Copyright (C) 2016 Arthur M\n");
    terminal_restorecolor();
    puts("\tLicensed under GNU GPL 2.0\n\n");

    if (!serial_init(0)) {
        puts("WARNING: Serial port not avaliable");
    }


    kprintf("\nMask: %x\n", pic_getmask());
    serial_putc('!');


    asm("sti");
    puts("=");

    for(;;) {
        asm volatile("nop");
    }
}
