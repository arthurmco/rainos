#include "ttys.h"

static uint8_t* default_color_ptr = 0;
static uint16_t x, y;
void ttys_init(terminal_t* term)
{
    /* Start serial port in early state */
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
        term->term_getx_f = &ttys_getx;
        term->term_gety_f = &ttys_gety;
        term->term_setx_f = &ttys_setx;

        term->term_gets = &ttys_gets;
        term->term_getc = &ttys_getc;
    }

    x = 0;
    y = 0;
}

void ttys_putc(unsigned char c)
{
    if (c == '\n') {
        /*  Sends a CRLF combination, for being compatible with most terminal
            clients */
        serial_putc('\r');
        y++;
    }

    serial_putc(c);
    x++;
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
    ttys_puts("\033[2J");

    y = 0;
    x = 0;
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

char ttys_getc()
{
    return serial_getc();
}

size_t ttys_gets(char* c, size_t len)
{
    y = 0;
    for (size_t i = 0; i < len; i++) {
        c[i] = serial_getc();

        if (c[i] == '\r') {
            serial_putc('\n');
            y++;
        } else if (c[i] == '\b' || c[i] == 0x7f){
            if (i > 0) {
                ttys_setx(x-1);
                serial_putc(' ');
                c[i] = ' ';
                ttys_setx(x-1);
                i -= 2;
            }
            continue;
        } else {
            serial_putc(c[i]);
            x++;
        }

        if (c[i] == '\r' || c[i] == '\n') {
            c[i] = '\n';
            c[i+1] = 0;
            return i+1;
        }
    }
}

uint16_t ttys_getx()
{
    return x;
}

uint16_t ttys_gety()
{
    return y;
}

void ttys_setx(uint16_t nx)
{
    int16_t delta = ((int16_t)nx)-x;
    char ansi_setx[16];

    if (delta > 0)
        sprintf(ansi_setx, "\033[%dC", delta);
    else
        sprintf(ansi_setx, "\033[%dD", -delta);

    ttys_puts(ansi_setx);
    x = nx;
}
