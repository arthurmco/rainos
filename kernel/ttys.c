#include "ttys.h"

static uint8_t* default_color_ptr = 0;
void ttys_init(terminal_t* term)
{
    if (!serial_early_init(0)) {
        return;
    }

    if (term) {
        term->term_putc_f = &ttys_putc;
        term->term_puts_f = &ttys_puts;
        term->term_clear_f = &ttys_clear;
        term->term_getcolor_f = &ttys_getcolor;
        term->term_setcolor_f = &ttys_setcolor;
        default_color_ptr = &term->defaultColor;

        if (*default_color_ptr)
            *default_color_ptr = 0x07;
    }
}

void ttys_putc(unsigned char c)
{
    if (c == '\n')
        /*  Sends a CRLF combination, for being compatible with most terminal
            clients */
        serial_putc('\r');

    serial_putc(c);
}
void ttys_puts(const char* s)
{
    while (*s) {
        ttys_putc(*s);
        s++;
    }
}
void ttys_clear()
{
    for (int i = 0; i < 25; i++)
        ttys_putc('\n');
}

static uint8_t _color = 0;
uint8_t ttys_getcolor()
{
    return _color;
}

/*  Character representation of ANSI escape codes for colouring
    The index are VGA colors for foreground */

static const char* colors_fg[] =
{
    "0;30", "0;34", "0;32", "0;36", "0;31", "0;35", "0;33", "0;37",
    "1;30", "1;34", "1;32", "1;36", "1;31", "1;35", "1;33", "1;37"
};

static const char* colors_bg[] =
{
    "0;40", "0;44", "0;42", "0;46", "0;41", "0;45", "0;43", "0;47",
    "1;40", "1;44", "1;42", "1;46", "1;41", "1;45", "1;43", "1;47"
};

void ttys_setcolor(uint8_t color)
{
    /* If color is default, then we take out the color modifier */
    if (color == *default_color_ptr) {
        ttys_puts("\033[0m");
        _color = *default_color_ptr;
        return;
    }

    uint8_t bg = color >> 4;
    uint8_t fg = color & 0xf;

    char color_str[32];
    sprintf(color_str, "\033[%s;%sm",
        bg == (*default_color_ptr >> 4) ? "" : colors_bg[bg],
        fg == (*default_color_ptr & 0xf) ? "" : colors_fg[fg]);
    ttys_puts(color_str);
    _color = color;
}
