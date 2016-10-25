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

void fault_trace(uint32_t ebp) {
    kprintf("\n\tTrace:\n");

    /* Stack starts at 0x100000 at least, so.... */
    /*TODO: Get CORRECT stack top */
    if (ebp < 0x100000 || ebp > 0x400000) {
        kprintf("FATAL: stack corrupted!\n");
        return;
    }

    uint32_t st = ebp;
    uint32_t eip = 0;
    int i = 0;
    do {
        uint32_t eip =  ((uint32_t*)st)[1];
        kprintf("\t\t <%08x> %08x\n", eip, eip);
        st = ((uint32_t*)st)[0];
        i++;

    } while (st >= 0x100000 || eip >= 0x100000);
}

void fault_handler(regs_t* r) {

    asm("cli");
    kputs("\n");
    kerror("Processor Exception: %s\n", fault_names[r->int_no]);

    kprintf("eax: %08x\t ebx: %08x\t ecx: %08x\t edx:%08x\n", r->eax, r->ebx, r->ecx, r->edx);
    knotice("eax: %08x\t ebx: %08x\t ecx: %08x\t edx:%08x", r->eax, r->ebx, r->ecx, r->edx);

    kprintf("eip: %08x\t esp: %08x\t ebp: %08x \n", r->eip, r->esp, r->ebp);
    knotice("eip: %08x\t esp: %08x\t ebp: %08x ", r->eip, r->esp, r->ebp);

    kprintf("esi: %08x\t edi: %08x \n", r->esi, r->edi);
    knotice("esi: %08x\t edi: %08x", r->esi, r->edi);

    /* Parse EFLAGS */
    char str_eflags[128];    str_eflags[0] = 0;
    sprintf(str_eflags, "IOPL(%d) |", (r->eflags >> 12) & 3);

    if (r->eflags & 0x1)        strcat(str_eflags, "CF ");  //carry flag
    if (r->eflags & 0x4)        strcat(str_eflags, "PF ");  // parity flag
    if (r->eflags & 0x10)       strcat(str_eflags, "AF "); // arithmetic flag
    if (r->eflags & 0x40)       strcat(str_eflags, "ZF "); // zero flag
    if (r->eflags & 0x80)       strcat(str_eflags, "SF "); // sign flag
    if (r->eflags & 0x100)      strcat(str_eflags, "TF "); // trap flag
    if (r->eflags & 0x200)      strcat(str_eflags, "IF "); // Interrupt enable flag
    if (r->eflags & 0x400)      strcat(str_eflags, "DF "); // direction flag
    if (r->eflags & 0x800)      strcat(str_eflags, "OF "); // overflow flag
    if (r->eflags & 0x4000)     strcat(str_eflags, "NT "); //nested task flag
    if (r->eflags & 0x10000)    strcat(str_eflags, "RF "); // resume flag
    if (r->eflags & 0x20000)    strcat(str_eflags, "VM "); //vm8086 flag

    kprintf("eflags: %08x (%s)\n", r->eflags, str_eflags);
    knotice("eflags: %08x (%s)", r->eflags, str_eflags);


    /** TODO: make special handles for faults, like how was made on IRQs */
    if (r->int_no == 14) {
        /* if page fault, get CR2 */
        uint32_t _cr2 = 0x0, _cr3 = 0x0;
        asm volatile("mov %%cr2, %%eax" : "=a"(_cr2));
        asm volatile("mov %%cr3, %%eax" : "=a"(_cr3));

        kprintf("cr2: %08x\t cr3: %08x\n", _cr2, _cr3);
        knotice("cr2: %08x\t cr3: %08x\n", _cr2, _cr3);
    }

    kprintf("\nexception code: %08x", r->err_code);

    /* Do not print a trace in double fault
        (the other exception might have ben fucked up the stack) */
    if (r->int_no != 8)
        fault_trace(r->ebp);

    asm("cli; hlt");

}
