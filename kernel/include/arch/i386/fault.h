/* Fault handlers

    Copyright (C) 2016 Arthur M
*/

#include "regs.h"
#include "idt.h"

#ifndef _FAULT_H
#define _FAULT_H

void fault_init();

void fault_handler(regs_t* r);


/*  The exceptions handlers goes */
extern void isr0();     //Divide by zero
extern void isr1();     //Debug
extern void isr2();     //Non-maskable interrupts
extern void isr3();     //Breakpoint
extern void isr4();     //Overflow
extern void isr5();     //Bound range exceeded
extern void isr6();     //Invalid opcode
extern void isr7();     //FPU not avaliable
extern void isr8();     //Double fault
extern void isr9();     //Coprocessor Segment Overrun (deprecated in 486)
extern void isr10();    //Invalid TSS
extern void isr11();    //GDT segment not present
extern void isr12();    //Stack segment fault
extern void isr13();    //General Protection fault
extern void isr14();    //Page fault



#endif /* end of include guard: _FAULT_H */
