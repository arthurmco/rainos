#include "framebuffer.h"

static framebuffer_t* current_fb;

static void _fb_plot_pixel24(uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);
static void _fb_draw_line24(uint16_t x1, uint16_t y1,
    uint16_t x2, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);
static void _fb_putc24(char c, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);
static void _fb_puts24(const char* s, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);

static void _fb_plot_pixel32(uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);
static void _fb_draw_line32(uint16_t x1, uint16_t y1,
    uint16_t x2, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);
static void _fb_putc32(char c, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);
static void _fb_puts32(const char* s, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);

int framebuffer_set(framebuffer_t* fb)
{
    current_fb = fb;

    if (fb->bpp == 24) {
        fb->fb_plot_pixel = &_fb_plot_pixel24;
        fb->fb_draw_line = &_fb_draw_line24;
        fb->fb_putc = &_fb_putc24;
        fb->fb_puts = &_fb_puts24;
    } else if (fb->bpp == 32){
        fb->fb_plot_pixel = &_fb_plot_pixel32;
        fb->fb_draw_line = &_fb_draw_line32;
        fb->fb_putc = &_fb_putc32;
        fb->fb_puts = &_fb_puts32;
    } else {
        kerror("framebuffer: %d bpp unsupported", fb->bpp);
        return 0;
    }

    return 1;
}

void framebuffer_plot_pixel(uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b)
    {
        if (current_fb->fb_plot_pixel)
            current_fb->fb_plot_pixel(x,y,r,g,b);
    }

static void _fb_plot_pixel24(uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b)
    {
        uint8_t* off = (((uint32_t)current_fb->fb_addr) + (y * current_fb->pitch) + (x * 3));
        off[0] = b;
        off[1] = g;
        off[2] = r;
    }
static void _fb_draw_line24(uint16_t x1, uint16_t y1,
    uint16_t x2, uint16_t y2,
    uint8_t r, uint8_t g, uint8_t b)
    {
        uint32_t end =  (((uint32_t)current_fb->fb_addr) + (y2 * current_fb->pitch) + (x2 * 3));
        uint32_t start = (((uint32_t)current_fb->fb_addr) + (y1 * current_fb->pitch) + (x1 * 3));
        uint8_t* off = (uint8_t*)start;
        do {
            off[0] = b;
            off[1] = g;
            off[2] = r;
            off += 3;

        } while (off <= end);
    }
static void _fb_putc24(char c, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b)
    {

    }
static void _fb_puts24(const char* s, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b)
    {

    }

static void _fb_plot_pixel32(uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b)
    {
        uint32_t* off = (((uint32_t)current_fb->fb_addr) + (y * current_fb->pitch) + (x * 4));
        *off = ((r & ((1 << current_fb->red_mask) -1)) << current_fb->red_off);
        *off |= ((g & ((1 << current_fb->green_mask) -1)) << current_fb->green_off);
        *off |= ((b & ((1 << current_fb->blue_mask) -1)) << current_fb->blue_off);
    }
static void _fb_draw_line32(uint16_t x1, uint16_t y1,
    uint16_t x2, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b)
    {

    }
static void _fb_putc32(char c, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b)
    {

    }
static void _fb_puts32(const char* s, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b)
    {

    }
