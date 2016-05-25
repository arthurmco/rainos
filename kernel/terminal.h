/* Terminal driver for ArtOS

    Copyright (C) 2016 Arthur M
*/

#include <stdint.h>
#include <stddef.h>

#ifndef _TERMINAL_H
#define _TERMINAL_H

/*  Structure that represents a terminal window
    This structure should be opaque */
typedef struct _Terminal {
    char name[12];
    uint8_t defaultColor;

    void (*term_putc_f)(unsigned char c);
    void (*term_puts_f)(const char* s);
    void (*term_clear_f)();
    uint16_t (*term_gety_f)();
    uint16_t (*term_getx_f)();
    void (*term_sety_f)(uint16_t y);
    void (*term_setx_f)(uint16_t x);
    uint8_t (*term_getcolor_f)();
    void (*term_setcolor_f)(uint8_t color);
} terminal_t;

void terminal_set(terminal_t*);
terminal_t* terminal_get();

void terminal_putc(unsigned char c);
void terminal_puts(const char* s);
void terminal_clear();
uint16_t terminal_gety();
uint16_t terminal_getx();
void terminal_sety(uint16_t y);
void terminal_setx(uint16_t x);
uint8_t terminal_getcolor();
void terminal_setcolor(uint8_t color);
void terminal_restorecolor();

#endif /* end of include guard: _TERMINAL_H */
