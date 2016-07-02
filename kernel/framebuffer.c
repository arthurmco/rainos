#include "framebuffer.h"
#include "vgafonts.h"

static framebuffer_t* current_fb;

static void _fb_plot_pixel24(uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);
static void _fb_draw_line24(uint16_t x1, uint16_t y1,
    uint16_t x2, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);
static void _fb_putc24(char c, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);

static void _fb_plot_pixel32(uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);
static void _fb_draw_line32(uint16_t x1, uint16_t y1,
    uint16_t x2, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);
static void _fb_putc32(char c, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);

int framebuffer_set(framebuffer_t* fb)
{
    current_fb = fb;

    if (fb->bpp == 24) {
        fb->fb_plot_pixel = &_fb_plot_pixel24;
        fb->fb_draw_line = &_fb_draw_line24;
        fb->fb_putc = &_fb_putc24;
    } else if (fb->bpp == 32){
        fb->fb_plot_pixel = &_fb_plot_pixel32;
        fb->fb_draw_line = &_fb_draw_line32;
        fb->fb_putc = &_fb_putc32;
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

void framebuffer_putc(char c, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b)
    {
        if (current_fb->fb_putc)
            current_fb->fb_putc(c, x, y, r, g, b);
    }

void framebuffer_puts(const char* s, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b)
    {
        uint16_t cx = x, cy = y;
        while (*s) {
            switch (*s) {
            case '\n':
                cy += FONT_HEIGHT;
            case '\r':
                cx = x;
                break;
            case '\t':
                cx += 4;
                break;
            default:
                framebuffer_putc(*s, cx, cy, r, g, b);
                break;
            }

            cx += FONT_WIDTH;

            if (cx >= current_fb->width) {
                cx = 0;
                cy += FONT_HEIGHT;
            }

            if (cy >= current_fb->height) {
                /* scroll */
            }

            s++;
        }

    }

framebuffer_t* framebuffer_get_data()
{
    return current_fb;
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
        uint8_t* font_start = (uint8_t*)&vgafont16[FONT_HEIGHT*c];

        for (unsigned fy = 0; fy < FONT_HEIGHT; fy++) {
            for (unsigned fx = 0; fx < FONT_WIDTH; fx++) {
                if (font_start[fy] & (1 << (7-fx)))
                    _fb_plot_pixel24(x+fx, y+fy, r, g, b);
            }
        }

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
        uint8_t* font_start = (uint8_t*)&vgafont16[FONT_HEIGHT*c];

        for (unsigned fy = 0; fy < FONT_HEIGHT; fy++) {
            for (unsigned fx = 0; fx < FONT_WIDTH; fx++) {
                if (font_start[fy] & (1 << (7-fx)))
                    _fb_plot_pixel32(x+fx, y+fy, r, g, b);
            }
        }
    }
