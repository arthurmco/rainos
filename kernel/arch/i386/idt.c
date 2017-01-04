#include <arch/i386/idt.h>
#include <kstdlog.h>

static struct idt_ptr idt_pointer;
static struct idt_descriptor idt_descs[IDT_MAX_DESCRIPTORS];

static int idt_length = 0;

void idt_init(){

    /* Adds pseudo-entries for the descriptors */
    for (int i = 0; i < IDT_MAX_DESCRIPTORS; i++)
        idt_addentry(i, 0, 0, 0);

    idt_register();
}

void idt_addentry(uint8_t index, uint16_t gdt_selector, uint32_t handler_addr, uint8_t attributes){
    idt_descs[index].offset_low = (handler_addr & 0xffff);
    idt_descs[index].gdt_selector = gdt_selector;
    idt_descs[index].attributes = attributes;
    idt_descs[index].offset_high = (handler_addr >> 16) & 0xffff;
    idt_descs[index].zero = 0;

    if (index >= idt_length)
        idt_length = index+1;
}

extern uint32_t idt_flush(struct idt_ptr*);

void idt_register(){
    idt_pointer.addr = (uint32_t) &idt_descs[0];
    idt_pointer.size = (sizeof(struct idt_descriptor) * IDT_MAX_DESCRIPTORS) - 1;

    idt_flush(&idt_pointer);
    knotice("IDT: created IDT at 0x%x, limit %d",
        idt_pointer.addr, idt_pointer.size);
}
