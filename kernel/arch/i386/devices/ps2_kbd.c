#include "ps2_kbd.h"

static unsigned kbd_port = 0;

/* Initialize the keyboard */
int kbd_init()
{
    i8042_flush_recv(1);
    i8042_flush_recv(2);
    uint8_t ret = 0;

    for (unsigned p = 1; p <= 2; p++) {
        /* Identify each device */
        i8042_send(0xF2, p);

        ret = i8042_recv(p);
        if (ret != 0xFA) {
            /* there's an error on this port */
            kwarn("ps2_kbd: received unexpected message %x from controller",
                ret);
            continue;
        }

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
    i8042_wait_send();

    ret = i8042_recv(kbd_port);
    if (ret != 0xFA) {
        kerror("ps2_kbd: Keyboard returned unexpected value %x", ret);
        return 0;
    }


    /* Set to scancode set 2 */
    scancode_sec:
    i8042_send(0xF0, kbd_port);
    i8042_wait_send();
    i8042_send(0x02, kbd_port);
    i8042_wait_send();

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

    i8042_flush_recv(kbd_port);
    timeout = 0;
    /* Check if the scancode is set */
    scancode_check:
    i8042_send(0xF0, kbd_port);
    i8042_wait_send();
    i8042_send(0x00, kbd_port);
    i8042_wait_send();

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
    i8042_flush_recv(kbd_port);

    timeout = 0;
    set_typematic:
    /* Set keyboard typematic rate to ~15 Hz, 500ms of delay */
    i8042_send(0xF3, kbd_port);
    i8042_wait_send();
    i8042_send(0x35, kbd_port);
    i8042_wait_send();

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
    i8042_send(0xF4, kbd_port);
    i8042_wait_send();

    ret = i8042_recv(kbd_port);
    if (ret != 0xFA) {
        kerror("ps2_kbd: Keyboard returned unexpected value %x", ret);
        return 0;
    }

    knotice("ps2_kbd: Keyboard ready on PS/2 port %d", kbd_port);
    return 1;
}

/*  Get a scancode.
    Since the key scancode can have multiple sizes, we return a uint32_t
*/
uint32_t kbd_get_scancode()
{

}
