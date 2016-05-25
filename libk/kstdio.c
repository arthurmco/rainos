#include <kstdio.h>
#include "../kernel/terminal.h"

void putc(char c)
{
    terminal_putc(c);
}

void puts(const char* s)
{
    terminal_puts(s);
}

void printf(const char* format, ...)
{

}
