/* Driver for the 8259 Programmable Interrupt Controller (PIC)

    Copyright (C) 2016 Arthur M
*/

#include <stdint.h>
#include "ioport.h"

#ifndef _8259_H
#define _8259_H

/*  On x86, we have 2 PICs, the master and the slave.
    The slave is linked to master's IRQ 2 */

// Here's the PIC IO ports
#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1


void pic_init();

/* Remap the interrupts 0-15 to IDT vectors 0x20-0x3f */
void pic_remap();

/* Send the End of Interrupt signal */
void pic_eoi();

/* Bitmask to you to mask the IRQ.
    The bit index is the IRQ you want to mask.
    For example:    number 2 masks IRQ 1, because 2 = 10
                    number 8 masks IRQ 3, because 8 = 1000
                    number 15 masks IRQs 0-3, because 15 = 1111 */
void pic_setmask(uint16_t bitmask);
uint16_t pic_getmask();

#endif /* end of include guard: _8259_H */
