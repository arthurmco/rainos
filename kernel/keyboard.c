#include "keyboard.h"
#include "arch/i386/devices/ps2_kbd.h"

/* Map of pressed/released keys by keycode.
    1 is pressed, 0 is released */
static uint8_t keys[KEY_MAX_KEYS];

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
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', ',', ':', '/', 0, '=',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
    'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
};

char kbd_get_ascii_key(struct key_event* ev)
{
    if (ev->key < KEY_MAX_KEYS && ev->key_status == KEYS_PRESSED) {
        return key2ascii_normal[ev->key];
    }

    return 0;
}
