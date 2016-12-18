/* Abstraction for x86 'in' and 'out' instructions */

#include <stdint.h>

#ifndef _IOPORT_H
#define _IOPORT_H


uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t data);

uint16_t inw(uint16_t port);
void outw(uint16_t port, uint16_t data);

uint32_t inl(uint16_t port);
void outl(uint16_t port, uint32_t data);

/* Waits some cycles for an IO operation to complete */
void io_wait();


#endif /* end of include guard: IOPORT_H */
