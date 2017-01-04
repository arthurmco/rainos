#include <time.h>

static struct time_tm t;
static void time_hook(regs_t* registers);

/*  Initialize time system with appropriate values.
    They can be extracted from some device, i.e the
    CMOS clock

    unix_time will be automatically calculated.
*/
void time_init(struct time_tm time)
{
    t = time;

    irq_add_handler(0, &time_hook);
}

void time_settime(struct time_tm time)
{
    t = time;
}
void time_gettime(struct time_tm* time_out)
{
    *time_out = t;
}


/*  Interrupt hook for updating the clock
    without needing to read the CMOS */
static void time_hook(regs_t* registers) {

    if ((pit_get_counter() % (1000*TICKS_PER_MS)) != 0)
        return;

    t.second++;
    if (t.second >= 60) {
        t.second = 0;
        t.minute++;
        if (t.minute >= 60) {
            t.minute = 0;
            t.hour++;
            if (t.hour >= 24) {
                t.hour = 0;
                t.day++;
            }
        }
    }
}

uint16_t month_day_count[] =
    {0,31,28,31,30,31,30,31,31,30,31,30,31};

uint16_t month_day_sum[] =
    {0,31,59,90,120,151,181,212,243,273,304,334,365};

uint64_t time_to_unix(struct time_tm* t)
{
    /* Unix time is the number of seconds since epoch (1/1/1970) */
    int y = t->year - 1970;

    uint64_t utime = t->second + (t->minute * 60) + (t->hour * 3600) +
        ((t->day) + month_day_sum[t->month-1] ) * 86400 +
        (y * 31536000);

    /* Add leap years */
    utime += (y / 4) * 86400;

    return utime;
}

void time_from_unix(uint64_t utime, struct time_tm* t) {
    uint16_t ye,mon,day,hour,min,sec;

    ye = (utime / 31536000);
    utime -= (ye / 4) * 86400;
    ye += 1970;

    utime %= 31536000;

    day = (utime / 86400);
    utime %= 86400;

    for (int i = 0; i <= 12; i++) {
        if (day < month_day_sum[i]) {
            mon = i;
            day -= month_day_sum[i-1];
            break;
        }
    }

    day += 1;

    hour = utime/3600;
    utime %= 3600;

    min = utime/60;
    utime %= 60;

    sec = utime;

    t->year = ye;
    t->month = mon;
    t->day = day;
    t->hour = hour;
    t->minute = min;
    t->second = sec;
}
