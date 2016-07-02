/*  Framebuffer console driver

    Copyright (C) 2016 Arthur M
*/

#include "terminal.h"
#include "framebuffer.h"

#ifndef _FBCON_H
#define _FBCON_H

int fbcon_init(terminal_t* term);

void fbcon_putc(unsigned char c);
void fbcon_puts(const char* s);
void fbcon_clear();
uint16_t fbcon_gety();
uint16_t fbcon_getx();
void fbcon_sety(uint16_t y);
void fbcon_setx(uint16_t x);
uint8_t fbcon_getcolor();
void fbcon_setcolor(uint8_t color);
void fbcon_restorecolor();

size_t fbcon_getwidth();
size_t fbcon_getheight();


#endif /* end of include guard: _FBCON_H */
