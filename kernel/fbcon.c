#include "fbcon.h"

static int charX = 0, charY = 0;
static int charW = 0, charH = 0;
static int usedColor, defaultColor;
static framebuffer_t* fb;

static uint8_t vga_fb[16][3] =
{{0,0,0}, {170,0,0}, {0,170,0}, {170,85,0}, {0,0,170}, {170,0,170}, {0,170,170},
 {170,170,170}, {85,85,85}, {255,85,85}, {85,255,85}, {255,255,85}, {85,85,255},
 {255,85,255}, {85,255,255}, {255,255,255}};

int fbcon_init(terminal_t* term)
{
    if (!framebuffer_get_data()) return 0;

    fb = framebuffer_get_data();
    charW = fb->width / FONT_WIDTH;
    charH = fb->height / FONT_HEIGHT;

    knotice("fbcon: framebuffer terminal of %dx%d chars", charW, charH);

    if (term) {
        term->name[0] = 'f';
        term->term_putc_f = &fbcon_putc;
        term->term_puts_f = &fbcon_puts;
        term->term_clear_f = &fbcon_clear;
        term->term_getcolor_f = &fbcon_getcolor;
        term->term_setcolor_f = &fbcon_setcolor;
        term->term_getwidth = &fbcon_getwidth;
        term->term_getheight = &fbcon_getheight;
        term->term_setx_f = &fbcon_setx;
        term->term_sety_f = &fbcon_sety;
        term->term_getx_f = &fbcon_getx;
        term->term_gety_f = &fbcon_gety;
    }

    defaultColor = term->defaultColor;
    usedColor = defaultColor;
    charX = 0;
    charY = 0;
    return 1;
}

void fbcon_scroll()
{
    for (size_t y = FONT_HEIGHT; y < fb->height; y++) {
        memcpy(((uintptr_t)fb->fb_addr) + fb->pitch * y,
               ((uintptr_t)fb->fb_addr) + fb->pitch * (y-FONT_HEIGHT),
               fb->pitch);
    }

    for (size_t y = fb->height-FONT_HEIGHT; y < fb->height; y++) {
        for (size_t x = 0; x < fb->width; x++) {
            framebuffer_plot_pixel(x, y, 0, 0, 0);
        }
    }

}

void fbcon_putc(unsigned char c)
{
    switch (c) {
    case '\n':
        charY++;
    case '\r':
        charX = 0;
        break;
    case '\t':
        charX += 4;
        break;
    default:
        framebuffer_putc(c, charX*FONT_WIDTH, charY*FONT_HEIGHT,
            vga_fb[usedColor&0xf][2], vga_fb[usedColor&0xf][1],
            vga_fb[usedColor&0xf][0]);
        charX++;
        break;
    }
    if (charX > charW) {
        charX = 0;
        charY++;
    }
    if (charY >= charH) {
        /* scroll */
        charY--;
        fbcon_scroll();
    }
}
void fbcon_puts(const char* s)
{
    while (*s) {
        fbcon_putc(*s);
        s++;
    }
}

void fbcon_clear()
{
    for (size_t y = 0; y < fb->height; y++) {
        for (size_t x = 0; x < fb->width; x++) {
            framebuffer_plot_pixel(x, y, 0, 0, 0);
        }
    }
    charX = 0;
    charY = 0;
}

uint16_t fbcon_gety() { return charY; }
uint16_t fbcon_getx() { return charX; }
void fbcon_sety(uint16_t y) { charY = y; }
void fbcon_setx(uint16_t x) { charX = x; }
uint8_t fbcon_getcolor() { return usedColor; }
void fbcon_setcolor(uint8_t color) { usedColor = color; }
void fbcon_restorecolor() { usedColor = defaultColor; }

size_t fbcon_getwidth()
{ return charW; }
size_t fbcon_getheight()
{ return charH; }
