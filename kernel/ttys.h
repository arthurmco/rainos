/*  Serial terminal device driver.
    Transforms serial port into a terminal device.

    Copyright (C) 2016 Arthur M */

#include <stdint.h>
#include <kstring.h>
#include "terminal.h"
#include "arch/i386/devices/serial.h"

#ifndef _TTYS_H
#define _TTYS_H

void ttys_init(terminal_t* term);
void ttys_putc(unsigned char c);
void ttys_puts(const char* s);
void ttys_clear();

uint8_t ttys_getcolor();
void ttys_setcolor(uint8_t color);


#endif /* end of include guard: _TTYS_H */
