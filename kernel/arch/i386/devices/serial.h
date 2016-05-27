/* Serial port driver for ArtOS

    Copyright (C) 2016 Arthur M
 */

#include "ioport.h"
#include "../bda.h"

#ifndef _SERIAL_H
#define _SERIAL_H


int serial_init();
int serial_set_baud(unsigned baud);

void serial_set_port(unsigned num);
unsigned serial_get_port();

void serial_putc(unsigned char);
char serial_getc();
int serial_isready();


#endif /* end of include guard: _SERIAL_H */
