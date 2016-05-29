# IRQ handling specific ASM bits
# Copyright (C) 2015-2016 Arthur M

.macro IRQ irqnum
   irq\irqnum\() :
   cli
   push $0         # Some code to allow me to reuse the exception handling regs_t structure
   push $\irqnum   # The exception number

   jmp irq_common # Jump to common ISR cdde
.endm

.global irq0
.global irq1
.global irq2
.global irq3
.global irq4
.global irq5
.global irq6
.global irq7
.global irq8
.global irq9
.global irq10
.global irq11
.global irq12
.global irq13
.global irq14
.global irq15

IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15

# External C function to handler IRQs
# void _irq_handler(registers_t* regs)
.extern irq_handler

irq_common:
   pushal  # Push registers and stack addresses
   push %ds # Push data segments
   push %es
   push %fs
   push %gs

   mov $0x10, %ax # Load the kernel data segment descriptor
   mov %ax, %ds
   mov %ax, %es
   mov %ax, %fs
   mov %ax, %gs

   mov %esp, %eax # We save the stack so we can restore it later
   push %eax

   movl $irq_handler, %eax
   call *%eax  # This preserves the EIP register

   # Undo everything IN REVERSE ORDER
   pop %eax
   pop %gs
   pop %fs
   pop %es
   pop %ds
   popal
   add $8, %esp # Clear the exception number and code
   iret    # Return from executing code before the interrupt call.
           # Restores CS, EIP, EFLAGS, SS and ESP.
