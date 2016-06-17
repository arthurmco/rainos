#include "fault.h"
#include <kstdio.h>

void fault_init() {
    //Adds the entries for all the 15 base exceptions
    idt_addentry(0, 0x08, (uint32_t)&isr0, IDTA_INTERRUPT);
    idt_addentry(1, 0x08, (uint32_t)&isr1, IDTA_INTERRUPT);
    idt_addentry(2, 0x08, (uint32_t)&isr2, IDTA_INTERRUPT);
    idt_addentry(3, 0x08, (uint32_t)&isr3, IDTA_INTERRUPT);
    idt_addentry(4, 0x08, (uint32_t)&isr4, IDTA_INTERRUPT);
    idt_addentry(5, 0x08, (uint32_t)&isr5, IDTA_INTERRUPT);
    idt_addentry(6, 0x08, (uint32_t)&isr6, IDTA_INTERRUPT);
    idt_addentry(7, 0x08, (uint32_t)&isr7, IDTA_INTERRUPT);
    idt_addentry(8, 0x08, (uint32_t)&isr8, IDTA_INTERRUPT);
    idt_addentry(9, 0x08, (uint32_t)&isr9, IDTA_INTERRUPT);
    idt_addentry(10, 0x08, (uint32_t)&isr10, IDTA_INTERRUPT);
    idt_addentry(11, 0x08, (uint32_t)&isr11, IDTA_INTERRUPT);
    idt_addentry(12, 0x08, (uint32_t)&isr12, IDTA_INTERRUPT);
    idt_addentry(13, 0x08, (uint32_t)&isr13, IDTA_INTERRUPT);
    idt_addentry(14, 0x08, (uint32_t)&isr14, IDTA_INTERRUPT);

}

/* Names */
static const char* fault_names[] =
    {
        "Divide by zero",
        "Debug",
        "Non-maskable Interrupt",
        "Breakpoint",
        "Overflow",
        "Bound range exceeded",
        "Invalid opcode",
        "FPU not found",
        "Double fault",
        "Coprocessor Segment Overrun",
        "Invalid TSS",
        "GDT segment not found",
        "Stack segment fault",
        "GPF",
        "Page fault",
        "Unknown Interrupt",
        "Unknown Interrupt",
        "Unknown Interrupt",
        "Unknown Interrupt",
        "Unknown Interrupt",
        "Unknown Interrupt",
        "Unknown Interrupt",
        "Unknown Interrupt",
        "Unknown Interrupt",
        "Unknown Interrupt"
    };

void fault_handler(regs_t* r) {
    kputs("\n");
    kerror("Processor Exception: %s\n", fault_names[r->int_no]);
    kprintf("eax: %08x\t ebx: %08x\t ecx: %08x\t edx:%08x\t\n", r->eax, r->ebx, r->ecx, r->edx);
    kprintf("eip: %08x\t esp: %08x\t ebp: %08x\t \n", r->eip, r->esp, r->ebp);

    if (r->int_no == 14) {
        /* if page fault, get CR2 */
        uint32_t _cr2 = 0x0, _cr3 = 0x0;
        asm volatile("mov %%cr2, %%eax" : "=a"(_cr2));
        asm volatile("mov %%cr3, %%eax" : "=a"(_cr3));

        kprintf("cr2: %08x\t cr3: %08x\n", _cr2, _cr3);
    }

    kprintf("\nexception code: %02x", r->err_code);

    asm("cli; hlt");
}
