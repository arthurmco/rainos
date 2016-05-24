#include "ioport.h"


uint8_t inb(uint16_t port)
{
    uint8_t data;
    asm( "inb %1, %0" : "=a"(data) : "Nd"(port) );

    return data;
}

void outb(uint16_t port, uint8_t data)
{
    asm( "outb %0, %1" : : "a"(data), "Nd"(port) );
}

uint16_t inw(uint16_t port)
{
    uint16_t data;
    asm( "inw %1, %0" : "=a"(data) : "Nd"(port) );

    return data;
}

void outw(uint16_t port, uint16_t data)
{
    asm( "outw %0, %1" : : "a"(data), "Nd"(port) );
}

uint32_t inl(uint16_t port)
{
    uint32_t data;
    asm( "inl %1, %0" : "=a"(data) : "Nd"(port) );

    return data;
}

void outl(uint16_t port, uint32_t data)
{
    asm( "outl %0, %1" : : "a"(data), "Nd"(port) );
}


/* Waits some cycles for an IO operation to complete */

void io_wait()
{
    volatile int a = 0;
    for (a = 0; a < 100000; a++) {
        a = ~a;
        a = ~a;
        __asm__("nop");
    }
}
