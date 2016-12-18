/*
    PS/2 keyboard driver

    Copyright (C) 2016 Arthur M

*/
#include <stdint.h>

#include "8042.h"
#include "../../../keyboard.h"


#ifndef _PS2_KBD
#define _PS2_KBD

/* Initialize the keyboard */
int kbd_init();

/*  Get a scancode.
    Since the key scancode can have multiple sizes, we return a uint32_t
*/
uint32_t kbd_get_scancode();


int kbd_scancode_to_key_event(uint32_t scan, struct key_event* key);

#endif /* end of include guard: _PS2_KBD */
