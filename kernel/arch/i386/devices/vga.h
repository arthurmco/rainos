/* VGA console driver for ArtOS */

#include <stdint.h>
#include "../../../terminal.h"

#ifndef _VGA_H
#define _VGA_H

#define VGA_BASE 0xb8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/* Helper macros */
//The first bakes a color, the second bakes an entry in VGA memory.
#define VGA_COLOR(bg, fg) ((bg << 4) | (fg & 0x0f))
#define VGA_ENTRY(c, color) ((color << 8) | (c & 0xff))

//Colors in VGA text mode palette
enum VGA_TM_COLORS {
    VGA_BLACK           = 0x0,
    VGA_BLUE            = 0x1,
    VGA_GREEN           = 0x2,
    VGA_CYAN            = 0x3,
    VGA_RED             = 0x4,
    VGA_MAGENTA         = 0x5,
    VGA_BROWN           = 0x6,
    VGA_LIGHTGREY       = 0x7,
    VGA_DARKGREY        = 0x8,
    VGA_LIGHTBLUE       = 0x9,
    VGA_LIGHTGREEN      = 0xA,
    VGA_LIGHTCYAN       = 0xB,
    VGA_LIGHTRED        = 0xC,
    VGA_LIGHTMAGENTA    = 0xD,
    VGA_YELLOW          = 0xE, //in some composite displays it can be brown
    VGA_WHITE           = 0xF,
};


void vga_init(terminal_t* term);

void vga_puts(const char* msg);
void vga_putc(unsigned char c);
void vga_putentry(uint16_t entry);

void vga_setx(uint16_t);
uint16_t vga_getx(void);
void vga_sety(uint16_t);
uint16_t vga_gety(void);

void vga_clear();
void vga_scrollup();


#endif
