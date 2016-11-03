/*  True task switching function

    Copyright (C) 2016 Arthur M.
*/

old_esp: .long 0
old_eip: .long 0

.long 0x54495453
/* prototype: task_true_switch(task_regs_t* old, task_regs_t* new);
*/
.global task_true_switch
.type task_true_switch, @function
.align 16
task_true_switch:
    cli                     # Disable interrupts

    /* The old register set */

    mov %esp, (old_esp)
    movl 4(%esp), %esp      # Get the old task registers
    pushal

    mov (old_esp), %eax
    mov %eax, 12(%esp)

    mov %cr3, %eax
    push %eax               # Copy the cr3
    pushfl

    mov $old_esp, %ebx
    mov -4(%ebx), %eax
    push %eax               # Get EIP

    /* The new register set */
    mov (old_esp), %esp
    movl 8(%esp), %esp

    add $4, %esp            # Skip EIP, the return address is already on stack.
    popfl
    pop %eax
    mov %eax, %cr3
    popal

    sti                     # Enable them again
ret
