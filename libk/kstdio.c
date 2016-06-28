#include <kstdio.h>
#include <kstring.h>
#include "../kernel/terminal.h"
#include "../kernel/keyboard.h"

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
    size_t i = 0;
    struct key_event kev;
    while (i < len) {
        kbd_get_event(&kev);

        char c = kbd_get_ascii_key(&kev);
        if (c == '\b') {

            if (i == 0)
                continue;

            i--;
            str[i] = 0;

            unsigned x = terminal_getx();
            if (x == 0) {
                terminal_sety(terminal_gety()-1);
            }
            terminal_setx(--x);
            terminal_putc(' ');
            x = terminal_getx();
            if (x == 0) {
                terminal_sety(terminal_gety()-1);
            }
            terminal_setx(--x);
        } else if (c == 0) {
            continue;
        } else if (c == '\r') {
            c = '\n';
            putc(c);
            str[i++] = c;
        } else {
            putc(c);
            str[i++] = c;
        }

        if (c == '\n') {
            break;
        }

    }

    str[i] = 0;
    return i;
}
