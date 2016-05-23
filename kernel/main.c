/* RainOS main kernel entry */

#include <stdint.h>

#include "arch/i386/devices/vga.h"

void kernel_main(uint32_t mboot) {
    vga_init();
    vga_clear();
    vga_puts("Test\nTest2\nTest3\tTest4");
    vga_puts("Test5\nTest6\nTest7\nTest8");
    vga_puts("Test9\nTest10\nTest11\nTest12");
    vga_puts("Test13\n\nTest6\nTest7\nTest8");
    vga_puts("Test13\n\nTe2st6\nTes2t7\nTest8");
    vga_puts("Test13\n\nTe2st6\nTe3s2t7\nT2e2st8");
    vga_puts("Test13\n\nT2est6\nTes2t7\nT2est8");
    vga_puts("Test13\n\nTe2s3t6\nT4est7\nTest28");
    vga_puts("Test13\n\nTest6\nTest7\nTe2st8");
 /*   vga_puts("Test13\n\nTest6\nTest7\nTest8");
    vga_puts("Test13\n\nTest6\nTest7\nTest8");
    vga_puts("Test13\n\nTest6\nTest7\nTest8");
    vga_puts("Test13\n\nTest6\nTest7\nTest8");
    vga_puts("Test23\n\nTest62\nTest72\nTes2t8");
   */ 
}