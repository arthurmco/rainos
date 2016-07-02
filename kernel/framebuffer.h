/*  Generic framebuffer interface

    Copyright (C) 2016 Arthur M
*/

#include <stdint.h>
#include <kstdlog.h>

#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

typedef struct framebuffer {
    /* Pixel masks */
    uint8_t blue_off, blue_mask;
    uint8_t green_off, green_mask;
    uint8_t red_off, red_mask;
    uint8_t alpha_off, alpha_mask;

    uint16_t width, height; /* Width and height, in pixels */
    uint16_t bpp;

    uint16_t pitch; /* Memory amount used by one line */

    void* fb_addr; /* Virtual address of the framebuffer */

    void (*fb_plot_pixel)(uint16_t x, uint16_t y,
        uint8_t r, uint8_t g, uint8_t b);
    void (*fb_draw_line)(uint16_t x1, uint16_t y1,
        uint16_t x2, uint16_t y,
        uint8_t r, uint8_t g, uint8_t b);
    void (*fb_putc)(char c, uint16_t x, uint16_t y,
        uint8_t r, uint8_t g, uint8_t b);
    void (*fb_puts)(const char* s, uint16_t x, uint16_t y,
        uint8_t r, uint8_t g, uint8_t b);

} framebuffer_t;

int framebuffer_set(framebuffer_t* fb);

void framebuffer_plot_pixel(uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);
void framebuffer_putc(char c, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);
void framebuffer_puts(const char* s, uint16_t x, uint16_t y,
    uint8_t r, uint8_t g, uint8_t b);

framebuffer_t* framebuffer_get_data();
#endif /* end of include guard: _FRAMEBUFFER_H */
