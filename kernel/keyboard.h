/* Keyboard support functions for the kernel */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

enum Keys {

    KEY_BACKSPACE = 0x0a,
    KEY_ENTER = 0x0d,
    KEY_SPACE = 0x0e,

    KEY_0 = 0x10,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,

    KEY_DOT = 0x1a,
    KEY_COMMA = 0x1b,
    KEY_COLON = 0x1c,
    KEY_SLASH = 0x1d,
    KEY_SHIFT = 0x1e,
    KEY_EQUAL = 0x1f,

    KEY_A = 0x20,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
};

enum KeyStatus {
    KEYS_PRESSED = 0x01,
    KEYS_RELEASED = 0x02,
};

struct key_event {
    uint32_t key;
    int key_status;
    struct __modif {
        unsigned is_shift:1;
        unsigned is_alt:1;
        unsigned is_ctrl:1;
        unsigned rsvd:5;
    } modifiers;
    uint8_t pad;
};


#endif /* end of include guard: _KEYBOARD_H */