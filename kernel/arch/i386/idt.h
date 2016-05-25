/* IDT base definition

    Copyright (C) 2016 Arthur M
*/

#include <stdint.h>

#ifndef _IDT_H
#define _IDT_H

struct idt_ptr {
    uint16_t size;
    uint32_t addr;
} __attribute__((packed));

struct idt_descriptor {
    uint16_t offset_low;     //The offset to interrupt handler
    uint16_t gdt_selector;
    uint8_t zero;
    uint8_t attributes;
    uint16_t offset_high;
} __attribute__((packed));

enum IDT_ATTRIBUTES {
    IDTA_PRESENT        = (1 << 7),
    IDTA_USERMODE       = (3 << 5),
    IDTA_TASKGATE       = 0x5,     //Defines 32-bit task gate
    IDTA_INTERRUPTGATE  = 0xE,     //Defines 32-bit interrupt gate
    IDTA_TRAPGATE       = 0xF  //Defines 32-bit trap gate
};

#define IDTA_INTERRUPT (IDTA_PRESENT | IDTA_INTERRUPTGATE)
#define IDTA_TASK (IDTA_PRESENT | IDTA_TASKGATE)
#define IDTA_TRAP (IDTA_PRESENT | IDTA_TRAPGATE)

/*  A trap gate does not save and restore EFLAGS
    A task gate is used for task switching */

#define IDT_MAX_DESCRIPTORS 64

/*  The exceptions handlers goes
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
 */
/* The IDT functions goes */
void idt_init();
void idt_addentry(uint8_t index, uint16_t gdt_selector, uint32_t handler_addr, uint8_t attributes);
void idt_register();


#endif /* end of include guard: _IDT_H */
