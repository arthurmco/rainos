/*
    PS/2 keyboard driver

    Copyright (C) 2016 Arthur M

*/

#include "8042.h"
#include <stdint.h>

#ifndef _PS2_KBD
#define _PS2_KBD

/* Initialize the keyboard */
int kbd_init();

/*  Get a scancode.
    Since the key scancode can have multiple sizes, we return a uint32_t
*/
uint32_t kbd_get_scancode();


#endif /* end of include guard: _PS2_KBD */
