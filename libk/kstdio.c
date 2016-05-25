#include <kstdio.h>
#include <kstring.h>
#include "../kernel/terminal.h"

void putc(char c)
{
    terminal_putc(c);
}

void puts(const char* s)
{
    terminal_puts(s);
}

void kprintf(const char* format, ...)
{
    char str[256];
    va_list vl;
    va_start(vl, format);
    vsprintf(str, format, vl);
    va_end(vl);
    puts(str);
}

void kerror(const char* format, ...) {
    terminal_setcolor(0x0c);
    terminal_puts("ERROR: ");
    terminal_restorecolor();

    char str[256];
    va_list vl;
    va_start(vl, format);
    vsprintf(str, format, vl);
    va_end(vl);
    puts(str);
}
