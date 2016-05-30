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

volatile int _timer = 0;
void timer(regs_t* regs) {
    kprintf("Timer: %d\n", _timer++);
}

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
    ttys_puts("\033[1mRainOS\033[0m\n");

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

    asm("sti");
    irq_add_handler(0, &timer);

    for(;;) {
        asm volatile("nop");
    }
}
