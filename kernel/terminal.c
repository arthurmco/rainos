#include "terminal.h"

/* The actual terminal, which the operations are made */
static terminal_t* term = NULL;

void terminal_set(terminal_t* t) {
    term = t;
}
terminal_t* terminal_get() {
    return term;
}

void terminal_putc(unsigned char c)
{
    if (term->term_putc_f)
        term->term_putc_f(c);
}
void terminal_puts(const char* s)
{

    if (term->term_puts_f) {
        term->term_puts_f(s);
    } else {
        while (*s)
            term->term_putc_f(*s++);
    }
}
void terminal_clear()
{
    if (term->term_clear_f)
        term->term_clear_f();
}
uint16_t terminal_gety()
{
    if (term->term_gety_f)
        return term->term_gety_f();
    else
        return 0;
}
uint16_t terminal_getx()
{
    if (term->term_getx_f)
        return term->term_getx_f();
    else
        return 0;

}
void terminal_sety(uint16_t y)
{
    if (term->term_sety_f)
        term->term_sety_f(y);

}
void terminal_setx(uint16_t x)
{
    if (term->term_setx_f)
        term->term_setx_f(x);

}
uint8_t terminal_getcolor()
{
    if (term->term_getcolor_f)
        return term->term_getcolor_f();
    else
        return 0;
}
void terminal_setcolor(uint8_t color)
{
    if (term->term_setcolor_f)
        term->term_setcolor_f(color);
}
void terminal_restorecolor() {
    terminal_setcolor(term->defaultColor);
}


size_t terminal_getwidth()
{
    if (term->term_getwidth)
        return term->term_getwidth();

    return 0;
}
size_t terminal_getheight()
{
    if (term->term_getheight)
        return term->term_getheight();

    return 0;
}

char terminal_getc()
{
    if (term->term_getc)
        return term->term_getc();

    return 0;
}

size_t terminal_gets(char* str, size_t len)
{
    if (term->term_gets)
        return term->term_gets(str, len);

    return 0;
}
