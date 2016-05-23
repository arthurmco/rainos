#include "vga.h"

static uint16_t* vga_addr = (uint16_t*) VGA_BASE;
static uint16_t xPos = 0;
static uint16_t yPos = 0;
uint8_t default_color;

void vga_init() {
    default_color = VGA_COLOR(VGA_BLACK, VGA_LIGHTGREY);    
}

void vga_puts(const char* msg) {
    while (*msg) {
        vga_putc(*msg++);
    }
}
void vga_putc(unsigned char c) {
   vga_putentry(VGA_ENTRY(c, default_color));    
}
void vga_putentry(uint16_t entry) {
    char c =  entry & 0xff;
     
    switch (c) {
        case '\n':
            yPos++;
        case '\r':
            xPos = 0;
            break;
        case '\t':
            xPos += 4;
            xPos = xPos & ~0x4;
            break;
        default:
            vga_addr[yPos*VGA_WIDTH+xPos] = entry;
            xPos++;
            break;            
    }
    
    if (xPos > VGA_WIDTH) {
        xPos = 0;
        yPos++;
    }
    
    if (yPos >= VGA_HEIGHT) {
        yPos = VGA_HEIGHT-1;
        vga_scrollup();
    }
    
}

void vga_setx(unsigned x){ xPos = x;}
unsigned vga_getx(void) { return xPos; }
void vga_sety(unsigned y) {yPos = y; }
unsigned vga_gety(void) { return yPos; }

void vga_clear() {
    for (unsigned i = 0; i < VGA_WIDTH*VGA_HEIGHT; i++) {
        vga_addr[i] = VGA_ENTRY(' ', default_color);
    }
}
void vga_scrollup() {
    unsigned y = 1;
    for (y = 1; y < VGA_HEIGHT; y++) {
        for (unsigned x = 0; x < VGA_WIDTH; x++) {
            vga_addr[(y-1)*VGA_WIDTH+x] = vga_addr[y*VGA_WIDTH+x];
        }
    }
    
    y = VGA_HEIGHT-1;
    for (unsigned x = 0; x < VGA_WIDTH; x++) {
       vga_addr[y*VGA_WIDTH+x] = VGA_ENTRY(' ', default_color);        
    }
    
}