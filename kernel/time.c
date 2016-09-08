#include "time.h"

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
