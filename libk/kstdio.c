#include <kstdio.h>
#include <kstring.h>

#include <terminal.h>
#include <keyboard.h>

void putc(char c)
{
    terminal_putc(c);
}

void kputs(const char* s)
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
    kputs(str);
}

size_t kgets(char* str, size_t len)
{
    return terminal_gets(str, len);
}

char kgetc()
{
    return terminal_getc();
}
