#include "pit.h"

void pit_setspeed(unsigned hz)
{
    if (hz == 0)
       hz++;

    //Divide the speed by the PIT base frequency
    uint16_t divisor = (1193180/hz) % 0xffff;

    //Select counter 0, write least, then most significant byte, use square wave and
    //use 16-bit counter
    outb(PIT_COMMAND_REGISTER, 0x36);
    outb(PIT_CHANNEL(0), divisor & 0xff);
    outb(PIT_CHANNEL(0), (divisor >> 8) & 0xff);

    knotice("8253 PIT: frequency set to %d, divisor %d", hz, divisor);
}

/* Counter that increments one unit at 1 milissecond */
volatile uint64_t pit_counter = 0;

static void pit_handler(regs_t* regs)
{
    pit_counter++;
}

uint64_t pit_get_counter()
{
    return pit_counter;
}


void pit_init()
{
    pit_setspeed(1000);

    irq_add_handler(0, &pit_handler);
}
