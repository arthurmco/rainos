#include <arch/i386/devices/ps2_kbd.h>

static unsigned kbd_port = 0;

/* Initialize the keyboard */
int kbd_init()
{
    i8042_flush_recv(1);
    i8042_flush_recv(2);
    uint8_t ret = 0;

    for (unsigned p = 1; p <= 2; p++) {
        io_wait();

        /* Identify each device */
        i8042_send(0xF2, p);
        i8042_wait_send();

        i8042_wait_recv();
        re_recv:
        ret = i8042_recv(p);
        if (ret != 0xFA) {
            /* there's an error on this port */
            kwarn("ps2_kbd: received unexpected message %x from controller",
                ret);
            if (i8042_wait_recv()) goto re_recv;
            continue;
        }

        io_wait();
        uint8_t dtype = i8042_recv(p);

        if (dtype == 0xAB) {
            kbd_port = p;
            knotice("ps2_kbd: detected keyboard on PS/2 port %d", p);
            dtype = i8042_recv(p);

            char* ktype;
            switch (dtype) {
                case 0x83:
                    ktype = "Translation disabled";
                    break;

                case 0xc1:
                case 0x41:
                    ktype = "Translation enabled";
                    break;

                default:
                    ktype = "Unknown";
                    break;
            }

            knotice("\tKeyboard type: %s", ktype);
        }

        if (kbd_port) break;
    }

    if (!kbd_port) {
        kerror("ps2_kbd: No keyboard ports found");
        return 0;
    }

    uint8_t timeout = 0;


    /* Disable scanning */
    i8042_send(0xF5, kbd_port);

    i8042_wait_recv();
    ret = i8042_recv(kbd_port);
    if (ret != 0xFA) {
        kerror("ps2_kbd: Keyboard returned unexpected value %x", ret);
        return 0;
	}

    /* Set to scancode set 2 */
    scancode_sec:
    i8042_send(0xF0, kbd_port);
    i8042_send(0x02, kbd_port);

    i8042_wait_recv();
    ret = i8042_recv(kbd_port);

    if (ret == 0xFE) {
        timeout++;

        if (timeout > 5) {
            kerror("ps2_kbd: Keyboard timed out");
            return 0;
        }

        goto scancode_sec;
    }

    if (ret != 0xFA) {
        kerror("ps2_kbd: Keyboard returned unexpected value %x", ret);
        return 0;
    }

    timeout = 0;
    /* Check if the scancode is set */
    scancode_check:
    i8042_send(0xF0, kbd_port);
    i8042_wait_send();
    i8042_send(0x00, kbd_port);

    i8042_wait_recv();
    ret = i8042_recv(kbd_port);

    if (ret == 0xFE) {
        timeout++;

        if (timeout > 5) {
            kerror("ps2_kbd: Keyboard timed out");
            return 0;
        }

        goto scancode_check;
    }

    if (ret != 0xFA) {
        kerror("ps2_kbd: Keyboard returned unexpected value %x", ret);
        return 0;
    }

    scancode_check_recv:
    i8042_wait_recv();
    ret = i8042_recv(kbd_port);

    if (ret == 0xfa) {
        goto scancode_check_recv; /* Delayed receive */
    }

    if (ret != 0x2) {
        kerror("ps2_kbd: Scan code value setup error, returned value 0x%x", ret);
        return 0;
    }

    knotice("ps2_kbd: Scancode set changed to set 2");

    timeout = 0;
    set_typematic:
    /* Set keyboard typematic rate to ~15 Hz, 500ms of delay */
    i8042_send(0xF3, kbd_port);
    i8042_wait_send();
    i8042_send(0x35, kbd_port);

    i8042_wait_recv();
    ret = i8042_recv(kbd_port);

    if (ret == 0xFE) {
        timeout++;

        if (timeout > 5) {
            kerror("ps2_kbd: Keyboard timed out");
            return 0;
        }

        goto scancode_check;
    }

    if (ret != 0xFA) {
        kerror("ps2_kbd: Keyboard returned unexpected value %x", ret);
        return 0;
    }


    /* Reenable scanning */
    i8042_wait_send();
    i8042_send(0xF4, kbd_port);

    i8042_wait_recv();
    ret = i8042_recv(kbd_port);
    if (ret != 0xFA) {
        kerror("ps2_kbd: Keyboard returned unexpected value %x", ret);
        return 0;
    }

    knotice("ps2_kbd: Keyboard ready on PS/2 port %d", kbd_port);
    return 1;
}

/*  Sometimes, multiple keys can come from a single request
    We need to buffer the extra keys somewhere.
*/
static uint32_t buffer[8];
static uint8_t iHead = 0, iTail = 0;

static int isShift, isCtrl, isAlt;

/*  Get a scancode.
    Since the key scancode can have multiple sizes, we return a uint32_t
    Returns the scancode, or 0 if no scancode.
*/
uint32_t kbd_get_scancode()
{

    if (iHead != iTail) {
        return buffer[iTail++];
    }

    if (iHead == iTail) {
        iHead = 0;
        iTail = 0;
    }

     /* Normal keys have 1 byte make code (xx) and 2 byte break code
        (F0 xx)

        Extended keys have 2 byte make code (E0 xx) and 3 byte break code
        (E0 F0 xx)

        Search for these patterns and break them
    */

    int key_ok = 0;
    int kextended = 0, kbreak = 0;
    uint32_t key;

    if (!i8042_check_recv()) {

        /* No key in buffer. */
        return 0;
    }

    while (!key_ok) {
        uint8_t key8 = i8042_recv(kbd_port);

        if (key8 == 0xF0) {
            kbreak = 1;
        } else if (key8 == 0xE0) {
            kextended = 1;
        } else {
            key = key8;
            key_ok = 1;
        }
    }

    if (kbreak) {
        key |= 0xF00000;
    }

    if (kextended) {
        key |= 0xE000;
    }

/*    knotice("key: %x, (%s)",
        key, (kbreak) ? "break" : "make"); */

    /* Flush buffer */
    buffer[iHead++] = key;

    i8042_flush_recv(kbd_port);

    return buffer[iTail++];
}

int kbd_scancode_to_key_event(uint32_t scan, struct key_event* key) {

    /*  Default keymap for en-US keyboards
        TODO: create a scancode to key translation table */
    switch (scan & 0xffff) {
        case 0x1C: key->key = KEY_A; break;
        case 0x32: key->key = KEY_B; break;
        case 0x21: key->key = KEY_C; break;
        case 0x23: key->key = KEY_D; break;
        case 0x24: key->key = KEY_E; break;
        case 0x2B: key->key = KEY_F; break;
        case 0x34: key->key = KEY_G; break;
        case 0x33: key->key = KEY_H; break;
        case 0x43: key->key = KEY_I; break;
        case 0x3B: key->key = KEY_J; break;
        case 0x42: key->key = KEY_K; break;
        case 0x4B: key->key = KEY_L; break;
        case 0x3A: key->key = KEY_M; break;
        case 0x31: key->key = KEY_N; break;
        case 0x44: key->key = KEY_O; break;
        case 0x4D: key->key = KEY_P; break;
        case 0x15: key->key = KEY_Q; break;
        case 0x2D: key->key = KEY_R; break;
        case 0x1B: key->key = KEY_S; break;
        case 0x2C: key->key = KEY_T; break;
        case 0x3C: key->key = KEY_U; break;
        case 0x2A: key->key = KEY_V; break;
        case 0x1D: key->key = KEY_W; break;
        case 0x22: key->key = KEY_X; break;
        case 0x35: key->key = KEY_Y; break;
        case 0x1A: key->key = KEY_Z; break;

        case 0x45: key->key = KEY_0; break;
        case 0x16: key->key = KEY_1; break;
        case 0x1e: key->key = KEY_2; break;
        case 0x26: key->key = KEY_3; break;
        case 0x25: key->key = KEY_4; break;
        case 0x2E: key->key = KEY_5; break;
        case 0x36: key->key = KEY_6; break;
        case 0x3D: key->key = KEY_7; break;
        case 0x3E: key->key = KEY_8; break;
        case 0x46: key->key = KEY_9; break;

        case 0x5A: key->key = KEY_ENTER; break;
        case 0x4A: key->key = KEY_SLASH; break;
        case 0x29: key->key = KEY_SPACE; break;
        case 0x66: key->key = KEY_BACKSPACE; break;
        case 0x49: key->key = KEY_DOT; break;
        case 0x12: key->key = KEY_LSHIFT; break;
    }

    key->key_status = (scan & 0xf00000) ? KEYS_RELEASED : KEYS_PRESSED;

    if (key->key == KEY_LSHIFT) {
        isShift = (key->key_status == KEYS_PRESSED);
    }

    key->modifiers.is_shift = isShift;
    return 1;

}
