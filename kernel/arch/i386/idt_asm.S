# IDT specific ASM bits
# Copyright (C) 2016 Arthur M

.global idt_flush
.type idt_flush, @function
idt_flush:
    # u32 idt_flush(struct idt_ptr* gdtptr)
    #   Registers the IDT into the processor
    #   Returns the pointer of idtptr.

    pushl   %ebp
    movl    %esp, %ebp

    # Gets the IDT pointer
    movl    8(%ebp), %ecx

    lidt (%ecx)

    movl    %ebp, %esp
    popl    %ebp
    ret

# The interrupt handlers
# Our exception handlers goes here
.global isr0  #Division by zero
.global isr1  #Debug
.global isr2  #Non-maskable interrupt
.global isr3  #Breakpoint
.global isr4  #Overflow
.global isr5  #Out of bounds
.global isr6  #Invalid Opcode
.global isr7  #No Coprocessor
.global isr8  #Double fault
.global isr9  #Coprocessor Segment Overrun
.global isr10  #Bad TSS
.global isr11  #Segment Not Present
.global isr12  #Stack fault
.global isr13  #GPF
.global isr14  #Page fault
.global isr15  #Unknown Interrupt

#Macro to insert an exception that doesn't push anything to the stack
.macro ISR_NOCODE isrnum
    isr\isrnum\() :
    #cli
    push $0x0       # Dummy error code
    push $\isrnum   # The exception number

    jmp isr_common # Jump to common ISR code
.endm

# Macro to insert an exception that doesn't push something to the stack
# (The Double fault exception is a good example)
.macro ISR_CODE isrnum
    isr\isrnum\() :
    cli
    push $\isrnum   # The exception number

    jmp isr_common # Jump to common ISR cdde
.endm

ISR_NOCODE 0
ISR_NOCODE 1
ISR_NOCODE 2
ISR_NOCODE 3
ISR_NOCODE 4
ISR_NOCODE 5
ISR_NOCODE 6
ISR_NOCODE 7
ISR_CODE 8
ISR_NOCODE 9
ISR_CODE 10
ISR_CODE 11
ISR_CODE 12
ISR_CODE 13
ISR_CODE 14
ISR_NOCODE 15


# External C function to treat kernel faults
# void _fault_handler(registers_t* regs)
.extern fault_handler

# Interrupt pushes eip, cs, eflags, user esp and ss automatically
isr_common:
    pushal  # Push registers (edi,esi,ebp,esp.ebx,edx.ecx.eax)
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

    movl $fault_handler, %eax
    call *%eax  # Bran say this preserves the EIP register

    # Undo everything IN REVERSE ORDER
    pop %eax
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popal
    add $8, %esp # Clear the exception number and code
    iret    # Return from executing code before the exception call.
            # Restores CS, EIP, EFLAGS, SS and ESP.
