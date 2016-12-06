/* Time and date management


   Copyright (C) 2016 Arthur M
*/

#include <stdint.h>
#include <kstdlog.h>

#include "arch/i386/irq.h"
#include "arch/i386/devices/pit.h"

#ifndef _TIME_H
#define _TIME_H

struct time_tm {
    int8_t timezone;
    uint16_t second, minute, hour, day, month;
    uint32_t year;
    uint64_t unix_time;
};

/*  Initialize time system with appropriate values.
    They can be extracted from some device, i.e the
    CMOS clock

    unix_time will be automatically calculated.
*/
void time_init(struct time_tm time);

void time_settime(struct time_tm time);
void time_gettime(struct time_tm* time_out);

uint64_t time_to_unix(struct time_tm* t);
void time_from_unix(uint64_t utime, struct time_tm* t);

#endif
