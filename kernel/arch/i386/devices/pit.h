/* 8253 Programmable Interval Timer driver

    Copyright (C) 2015-2016 Arthur M
*/

#include <kstdlog.h>
#include "ioport.h"
#include "../irq.h"

#ifndef _PIT_H
#define _PIT_H

/*  The programmable interval timer is a timer that shots an IRQ 0 at
    a fixed interval. It operates at 1,19318 MHz divided by a specified
    divisor. Our divisor will be 1193, so it shots averagely
    1 time per milisecond */

//PIT ports
#define PIT_CHANNEL(x) (0x40+x)
#define PIT_COMMAND_REGISTER 0x43

void pit_init();
void pit_setspeed(unsigned hz);

uint64_t pit_get_counter();


#endif /* end of include guard: _PIT_H */
