/* RainOS main kernel entry */

#include <stdint.h>

#include "arch/i386/devices/vga.h"
#include "terminal.h"

void kernel_main(uint32_t mboot) {
    terminal_t term_stdio;
    terminal_set(&term_stdio);
    vga_init(&term_stdio);

    terminal_puts("Hello world!");

}
