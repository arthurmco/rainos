#include "8259.h"

void pic_init()
{
    pic_remap();
    pic_setmask(0xffff);
}

/* Remap the interrupts 0-15 to IDT vectors 0x20-0x3f */
void pic_remap()
{
    //ICW1 - the initialisation command itself
    outb(PIC1_CMD, 0x11);
    io_wait();

    outb(PIC2_CMD, 0x11);
    io_wait();

    //ICW2 - interrupt vectors
    //Actually map the IRQ to 0x20 and 0x28 IDT entries
    outb(PIC1_DATA, 0x20);
    io_wait();

    outb(PIC2_DATA, 0x28);
    io_wait();

    //ICW3 - cascading mode
    outb(PIC1_DATA, 1 << 2); //we have slave PIC cascaded at IRQ 2 (the 1 is at bit 2)
    io_wait();

    outb(PIC2_DATA, 1 << 1); //the slave is the first cascaded IRQ (0000 0010)
    io_wait();

    //ICW4 - operation mode
    outb(PIC1_DATA, 1); //operate in 8086/8088 mode
    io_wait();

    outb(PIC2_DATA, 1); //idem the above
    io_wait();


}


/* Send the End of Interrupt signal */
void pic_eoi()
{

}


/* Bitmask to you to mask the IRQ.
    The bit index is the IRQ you want to mask, preventing it from firing.
    For example:    number 2 masks IRQ 1, because 2 = 10
                    number 8 masks IRQ 3, because 8 = 1000
                    number 15 masks IRQs 0-3, because 15 = 1111 */
void pic_setmask(uint16_t bitmask)
{
    /*  Sets the Interrupt Mask Register.
        It defines the IRQs that'll be sent to CPU */
    outb(PIC1_DATA, bitmask & 0xff);
    outb(PIC2_DATA, bitmask >> 8);
}
uint16_t pic_getmask()
{
    /*  Gets the Interrupt Mask Register. */
    uint8_t pic1, pic2;
    pic1 = inb(PIC1_DATA);
    io_wait();
    pic2 = inb(PIC2_DATA);

    return (pic1) | ((uint16_t)pic2 << 8);

}
