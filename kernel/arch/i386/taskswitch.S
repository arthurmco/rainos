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
                         # Disable interrupts

    /* The old register set */

    mov %esp, (old_esp)
    movl 4(%esp), %esp      # Get the old task registers
    pushal

    mov (old_esp), %eax
    mov %eax, -12(%esp)

    mov %cr3, %eax
    push %eax               # Copy the cr3
    pushfl

    mov $old_esp, %ebx
    mov 4(%ebp), %eax
    push %eax               # Get EIP

    cli
    hlt

    /* The new register set */
    mov (old_esp), %esp
    movl 8(%esp), %esp

    pop %eax            # Get the EIP and put it at the stack
    mov 16(%esp), %ebx  # so, when we return back, we'll return to that function

    movl %eax, (%ebx)   # Put the EIP
    movl $0x08, 4(%ebx) # The SS
    movl (%esp), %eax     # The EFLAGS
    movl %eax, 8(%ebx)
    movl 20(%esp), %eax     # The ESP
    movl %eax, 12(%ebx)
    movl $0x10, 16(%ebx)    # The SS


    popfl
    pop %eax
    mov %eax, %cr3
    popal

# Enable them again
iret
