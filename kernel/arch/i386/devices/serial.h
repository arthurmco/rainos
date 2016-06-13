/* Serial port driver for ArtOS

    Copyright (C) 2016 Arthur M
 */

#include "ioport.h"
#include "../bda.h"

#ifndef _SERIAL_H
#define _SERIAL_H

/* Early serial init, for kernel logging */
int serial_early_init(unsigned comdev);

int serial_set_baud(unsigned baud);

void serial_set_port(unsigned num);
unsigned serial_get_port();

void serial_putc(unsigned char);
char serial_getc();
int serial_isready();

#include "../../../dev.h"

int serial_init();

#endif /* end of include guard: _SERIAL_H */
