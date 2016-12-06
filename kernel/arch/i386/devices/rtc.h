/*
    x86 real time clock driver
    Copyright (C) 2016 Arthur M
*/

#include "../../../time.h"
#include "ioport.h"

#ifndef _RTC_H
#define _RTC_H

#define RTC_INDEX_PORT 0x70
#define RTC_DATA_PORT 0x71

#define BCD_TO_BINARY(bcd) (((bcd / 16) * 10) + (bcd & 0xf))

/* Initialize the system clock by getting rtc values */
void rtc_init();

/* Update the system clock */
void rtc_update();



#endif /* end of include guard: _RTC_H */
