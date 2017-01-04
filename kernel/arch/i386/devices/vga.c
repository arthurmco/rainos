#include <arch/i386/devices/vga.h>
#include <arch/i386/ebda.h>

static volatile uint16_t* vga_addr = (uint16_t*) VGA_BASE;
static uint16_t xPos = 0;
static uint16_t yPos = 0;
static uint8_t default_color;

static size_t vga_width() {return 80;}
static size_t vga_height() {return 25;}

void vga_init(terminal_t* term) {
    default_color = VGA_COLOR(VGA_BLACK, VGA_LIGHTGREY);

    if (term) {
        term->defaultColor = default_color;
        term->term_putc_f = &vga_putc;
        term->term_puts_f = &vga_puts;
        term->term_clear_f = &vga_clear;
        term->term_getx_f = &vga_getx;
        term->term_setx_f = &vga_setx;
        term->term_gety_f = &vga_gety;
        term->term_sety_f = &vga_sety;
        term->term_getcolor_f = &vga_getcolor;
        term->term_setcolor_f = &vga_setcolor;

        term->term_getheight = &vga_height;
        term->term_getwidth = &vga_width;
    }

}

static void _vga_update_cursor()
{
    unsigned short io_vga_base = bda_ptr->io_video_base ;

    uint16_t position = (yPos*80) + xPos;

    // cursor LOW port to vga INDEX register
    outb(io_vga_base, 0x0F);
    outb(io_vga_base+1, (uint8_t)(position&0xFF));

    // cursor HIGH port to vga INDEX register
    outb(io_vga_base, 0x0E);
    outb(io_vga_base+1, (uint8_t)((position>>8)&0xFF));
}

static void _vga_putc(unsigned char c)
{
    vga_putentry(VGA_ENTRY(c, default_color));
}

void vga_putc(unsigned char c) {
    _vga_putc(c);
    _vga_update_cursor();
}

void vga_puts(const char* msg) {
    while (*msg) {
        _vga_putc(*msg++);
    }
    _vga_update_cursor();
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
            if (xPos > 4)
                xPos = xPos & ~0x3;
            else
                xPos = 4;
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

void vga_setx(uint16_t x){ xPos = x; _vga_update_cursor(); }
uint16_t vga_getx(void) { return xPos; }
void vga_sety(uint16_t y) {yPos = y; _vga_update_cursor(); }
uint16_t vga_gety(void) { return yPos; }

void vga_clear() {
    for (unsigned i = 0; i < VGA_WIDTH*VGA_HEIGHT; i++) {
        vga_addr[i] = VGA_ENTRY(' ', default_color);
    }
    xPos = 0;
    yPos = 0;
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

uint8_t vga_getcolor() {return default_color;}
void vga_setcolor(uint8_t color) {default_color = color;}
