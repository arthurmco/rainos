#include <keyboard.h>
#include "arch/i386/devices/ps2_kbd.h"

/* Map of pressed/released keys by keycode.
    1 is pressed, 0 is released */
static uint8_t keys[KEY_MAX_KEYS];

static int shift = 0;

/* Get key state for a specific key.
    1 = Pressed, 0 = Released */
int kbd_get_key_state(uint32_t key)
{
    return keys[key];
}

int kbd_get_event(struct key_event* ev)
{
    uint32_t sc = 0;

    do {
        sc = kbd_get_scancode();
        _halt();

    } while (!sc);
    kbd_scancode_to_key_event(sc, ev);
    return 1;
}

int kbd_get_key()
{
    uint32_t sc = 0;
    struct key_event ke;

    do {
        sc = kbd_get_scancode();
        _halt();

    } while (!sc);

    kbd_scancode_to_key_event(sc, &ke);
    return ke.key;
}

static const char key2ascii_normal[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\b', 0, 0, '\r', ' ', 0,
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', ',', ';', '/', 0, '=',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
    'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
};

static const char key2ascii_shift[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\b', 0, 0, '\r', ' ', 0,
    ')', '!', '@', '#', '$', '%', '~', '&', '*', '(', '>', '<', ':', '/', 0, '+',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

char kbd_get_ascii_key(struct key_event* ev)
{
    if (ev->key < KEY_MAX_KEYS && ev->key_status == KEYS_PRESSED) {
        if (ev->modifiers.is_shift) {
            return key2ascii_shift[ev->key];
        }
        return key2ascii_normal[ev->key];
    }

    return 0;
}

static size_t kbd_gets(char* str, size_t len)
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

static char kbd_getc()
{
    struct key_event kev;

    do { kbd_get_event(&kev); _halt(); } while(kev.key_status != KEYS_PRESSED);

    return kbd_get_ascii_key(&kev);
}



void keyboard_init(terminal_t* term)
{
    if (term) {
        term->term_getc = &kbd_getc;
        term->term_gets = &kbd_gets;
    }
}
