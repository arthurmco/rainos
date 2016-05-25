/* RainOS main kernel entry */

#include <stdint.h>

#include "arch/i386/devices/vga.h"
#include "arch/i386/devices/ioport.h"
#include "terminal.h"

#include <kstdlib.h>
#include <kstring.h>

void kernel_main(uint32_t mboot) {
    terminal_t term_stdio;
    terminal_set(&term_stdio);
    vga_init(&term_stdio);

    terminal_clear();
    terminal_puts("Hello world!\n");

    int a = 0xf3096;
    char s[60];
    kprintf("Doutor %s e Maestro %s", "Pirrola", "Boquete");

}
