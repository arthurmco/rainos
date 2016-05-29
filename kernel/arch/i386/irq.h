/* IRQ handling and setup routines

    Copyright (C) 2016 Arthur M.
*/

#include <stddef.h>
#include "devices/8259.h"
#include "idt.h"
#include "regs.h"

#ifndef _IRQ_H
#define _IRQ_H

/* IRQ handler function */
typedef void(*irq_handler_f)(regs_t*);


/* Init IRQ handlers, setup IRQ sources (PIC or APIC) */
void irq_init();

/* IRQ general handler. It calls the other handlers you added */
void irq_handler(regs_t* regs);

int irq_add_handler(int irq, irq_handler_f func);
int irq_remove_handler(irq_handler_f func, int irq);


extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

#endif /* end of include guard: _IRQ_H */
