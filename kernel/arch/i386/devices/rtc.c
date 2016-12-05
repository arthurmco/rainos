#include "rtc.h"

/* Initialize the system clock by getting rtc values */
void rtc_init()
{
    struct time_tm t;

    /*  Get status register B values. It tell us how the date is encoded
        inside the CMOS chip */
    outb(RTC_DATA_PORT, 0xB);
    uint8_t statusB = inb(RTC_DATA_PORT);

    uint8_t isBinary = (statusB & 0x4);

    outb(RTC_INDEX_PORT, 0x0);
    t.second = isBinary ? inb(RTC_DATA_PORT) : BCD_TO_BINARY(inb(RTC_DATA_PORT));

    outb(RTC_INDEX_PORT, 0x2);
    t.minute = isBinary ? inb(RTC_DATA_PORT) : BCD_TO_BINARY(inb(RTC_DATA_PORT));

    outb(RTC_INDEX_PORT, 0x4);
    t.hour = isBinary ? inb(RTC_DATA_PORT) : BCD_TO_BINARY(inb(RTC_DATA_PORT));

    if (!(statusB & 0x1)) {
        if (t.hour & 0x80) {
            /* 12 hour clock format detected. Convert it to 24 */
            t.hour &= 32;   // take off that damn bit.
            t.hour += 12;
        }
    }

    outb(RTC_INDEX_PORT, 0x7);
    t.day = isBinary ? inb(RTC_DATA_PORT) : BCD_TO_BINARY(inb(RTC_DATA_PORT));

    outb(RTC_INDEX_PORT, 0x8);
    t.month = isBinary ? inb(RTC_DATA_PORT) : BCD_TO_BINARY(inb(RTC_DATA_PORT));

    outb(RTC_INDEX_PORT, 0x9);
    t.year = 2000 + (isBinary ? inb(RTC_DATA_PORT) : BCD_TO_BINARY(inb(RTC_DATA_PORT))); // cmos stores 2 digit years.

    time_settime(t);
    knotice("rtc: status register B is 0x%x", statusB);
    knotice("rtc: time is %02d:%02d:%02d %02d/%02d/%04d (h:m:s d/M/Y)",
        t.hour, t.minute, t.second, t.day, t.month, t.year);

    time_settime(t);
}

/* Update the system clock */
void rtc_update()
{
    rtc_init();
}
