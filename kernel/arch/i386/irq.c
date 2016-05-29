#include "irq.h"


/* Init IRQ handlers, setup IRQ sources (PIC or APIC) */
void irq_init() {
    pic_init();

    idt_addentry(0x20, 0x08, (uint32_t)&irq0, IDTA_INTERRUPT);
    idt_addentry(0x21, 0x08, (uint32_t)&irq1, IDTA_INTERRUPT);
    idt_addentry(0x22, 0x08, (uint32_t)&irq2, IDTA_INTERRUPT);
    idt_addentry(0x23, 0x08, (uint32_t)&irq3, IDTA_INTERRUPT);
    idt_addentry(0x24, 0x08, (uint32_t)&irq4, IDTA_INTERRUPT);
    idt_addentry(0x25, 0x08, (uint32_t)&irq5, IDTA_INTERRUPT);
    idt_addentry(0x26, 0x08, (uint32_t)&irq6, IDTA_INTERRUPT);
    idt_addentry(0x27, 0x08, (uint32_t)&irq7, IDTA_INTERRUPT);
    idt_addentry(0x28, 0x08, (uint32_t)&irq8, IDTA_INTERRUPT);
    idt_addentry(0x29, 0x08, (uint32_t)&irq9, IDTA_INTERRUPT);
    idt_addentry(0x2A, 0x08, (uint32_t)&irq10, IDTA_INTERRUPT);
    idt_addentry(0x2B, 0x08, (uint32_t)&irq11, IDTA_INTERRUPT);
    idt_addentry(0x2C, 0x08, (uint32_t)&irq12, IDTA_INTERRUPT);
    idt_addentry(0x2D, 0x08, (uint32_t)&irq13, IDTA_INTERRUPT);
    idt_addentry(0x2E, 0x08, (uint32_t)&irq14, IDTA_INTERRUPT);
    idt_addentry(0x2F, 0x08, (uint32_t)&irq15, IDTA_INTERRUPT);
}

#define MAX_IRQS 15
#define MAX_HANDLERS 15
static irq_handler_f handlers[MAX_IRQS][MAX_HANDLERS];
static int handler_pos[MAX_IRQS] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/* IRQ general handler. It calls the other handlers you added */
void irq_handler(regs_t* regs)
{
    for (int i = 0; i < MAX_IRQS; i++) {
        for (int h = 0; h < handler_pos[i]; h++) {
            irq_handler_f fun = handlers[i][h];
            if (fun)
                fun(regs);
        }
    }
}

int irq_add_handler(int irq, irq_handler_f func)
{
    if (!func) return 0;
    if (irq > MAX_IRQS) return 0;

    int h = handler_pos[irq];
    if (h > MAX_HANDLERS) return 0;

    handlers[irq][h] = func;

    handler_pos[irq] = ++h;
    return 1;
}

int irq_remove_handler(irq_handler_f func, int irq)
{
    if (!func) return 0;
    uintptr_t fptr = (uintptr_t)func;

    for (int h = 0; h < handler_pos[irq]; h++) {
        if ((uintptr_t)handlers[irq][h] == fptr) {
            handlers[irq][h] = NULL;
            if (h == (handler_pos[irq]-1)) {
                handler_pos[irq]--;
            }
            return 1;
        }
    }
    return 0;
}
