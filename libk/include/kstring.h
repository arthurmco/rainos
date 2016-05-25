/* String utilities used on the kernel

    Copyright (C) 2016 Arthur M.  */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#ifndef _KSTRING_H
#define _KSTRING_H

void strrev(const char* str, char* rev);
size_t strlen(const char* s);

void sprintf(char* str, const char* fmt, ...);

#endif /* end of include guard: _KSTRING_H */
