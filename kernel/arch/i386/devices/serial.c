#include "serial.h"

enum SERIAL_REGISTERS {
    SREG_DATA   = 0,
    SREG_INTENABLEREG = 1,
    /* When the Divisor Latch Access bit is set, the data and enable interrupt
       registers are used to control the UART (roughly, the serial port
       controller) frequency divisor.
       The DLAB is the most significant bit of the line control register
       (send a 0x80 to enable it ) */
    #define SREG_DIVISOR_LSB SREG_DATA
    #define SREG_DIVISOR_MSB SREG_INTENABLEREG
    SREG_FIFOCONTROL = 2,
    SREG_LINECONTROL = 3,
    SREG_MODEMCONTROL = 4,
    SREG_LINESTATUS = 5,
    SREG_MODEMSTATUS = 6,
    SREG_SCRATCH = 7,
};

static unsigned _portindex = 0;
#define PORT_COM(x) (bda_ptr->com_port[x])

int serial_early_init(unsigned port)
{
    _portindex = port;
    if (_portindex > 4)
        return 0;

    if (PORT_COM(_portindex) == 0)
        return 0;

    outb(PORT_COM(_portindex)+SREG_INTENABLEREG, 0x00);  // Disable all interrupts

    /* Set baud rate to 9600 */
    serial_set_baud(9600);

    outb(PORT_COM(_portindex)+SREG_LINECONTROL, 0x03); //Send 8 bits, no parity and one stop (aka sync) bit

    //Enable and clear the UART FIFO buffers, and set the overflow threshold to 16 bytes
    outb(PORT_COM(_portindex)+SREG_FIFOCONTROL, 0xC7);
    outb(PORT_COM(_portindex)+SREG_MODEMCONTROL, 0x0B); // IRQs enabled, RTS/DSR set



    return 1;
}

/*  Base frequency, that will be divided by the serial controller to
    result in (more or less) your desired frequency */
#define SERIAL_BASE 115200
int serial_set_baud(unsigned baud)
{
    unsigned divisor = SERIAL_BASE / baud;
    if (divisor == 0) return 0;

    outb(PORT_COM(_portindex)+SREG_LINECONTROL, 0x80); //Set the div. latch access bit
    outb(PORT_COM(_portindex)+SREG_DIVISOR_LSB, divisor & 0xff); //Use 12 as the divisor
    outb(PORT_COM(_portindex)+SREG_DIVISOR_MSB, divisor >> 8); //because 115200 / 12 = 9600
    outb(PORT_COM(_portindex)+SREG_LINECONTROL, 0x00); //Unset the div. latch access bit
}


void serial_putc(unsigned char c)
{
    /*  Check if we're ready to send.
        If we reach a timeout, do not send anything */
    int write_ok = 0;
    for (int i = 0; i < 10; i++) {
        /* is the Empty Transmitter Holding Register set? */
        if (inb(PORT_COM(_portindex)+SREG_LINESTATUS) & 0x20) {
            /* yes! we can transfer */
            write_ok = 1;
            break;
        }
        io_wait();
    }

    if (write_ok) {
        outb(PORT_COM(_portindex)+SREG_DATA, c);
    }
}

char serial_getc()
{
    while (!serial_isready()) {
        io_wait();
    }

    return inb(PORT_COM(_portindex)+SREG_DATA);
}

/* Is it ready to receive? */
int serial_isready()
{
    return (inb(PORT_COM(_portindex)+SREG_LINESTATUS) & 0x01);
}

/* Set port index, starting from 0.
    Ex: to set to COM1, you would write serial_set_port(0) */
void serial_set_port(unsigned num) {_portindex = num;}
unsigned serial_get_port() {return _portindex;}
