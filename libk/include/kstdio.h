/* Kernel functions used for I/O

    Copyright (C) 2016 Arthur M
*/

#ifndef _KSTDIO_H
#define _KSTDIO_H


#include <stdarg.h>

void putc(char c);
void kputs(const char* s);
void kprintf(const char* format, ...);
void kerror(const char* format, ...);


#endif /* end of include guard: _KSTDIO_H */
