/* BIOS data area attributes

    Copyright (C) 2016 Arthur M
*/

#include <stdint.h>

#ifndef _BDA_H
#define _BDA_H

struct BDA {
    uint16_t com_port[4]; /* COM1-COM4 */
    uint16_t lpt_port[3]; /* LPT1-LPT3 */
    uint16_t ebda_base; /* EBDA base >> 4 */
    uint16_t detected_hw_packed;
    uint8_t pad0[5];
    uint16_t kbd_state_flags;
    uint8_t pad1[5];
    uint8_t kbd_buffer[32];
    uint8_t pad2[11];
    uint8_t display_mode;
    uint16_t text_mode_cols;
    uint8_t pad3[23];
    uint16_t io_video_base;
    uint16_t irq0_count;
    uint8_t detected_hdd;
    uint16_t kbd_buffer_start;
    uint16_t kbd_buffer_end;
    uint8_t kbd_last_state;
} __attribute__((packed));

static volatile struct BDA* const bda_ptr = (volatile struct BDA* const)0x400;



#endif /* end of include guard: _BDA_H */
