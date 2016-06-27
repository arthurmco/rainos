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
