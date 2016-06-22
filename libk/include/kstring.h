/* String utilities used on the kernel

    Copyright (C) 2016 Arthur M.  */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#ifndef _KSTRING_H
#define _KSTRING_H

void strrev(const char* str, char* rev);
size_t strlen(const char* s);
char* strcat(char* str, const char* catted);

void sprintf(char* str, const char* fmt, ...);
void vsprintf(char* str, const char* fmt, va_list vl);

#define strcmp(s1, s2) strncmp(s1, s2, strlen(s1))
int strncmp(const char* s1, const char* s2, size_t len);

char* strtok(const char* s, char tok);
#endif /* end of include guard: _KSTRING_H */
