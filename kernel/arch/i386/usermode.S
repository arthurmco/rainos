/*  Usermode jumping function

    Copyright (C) 2016 Arthur M.
    */

#    C prototype:
#     void jump_usermode(uintptr_t stack, void (*usermode_function)()) */
.global jump_usermode
.type jump_usermode, @function
jump_usermode:
  movw $0x23, %ax   # 0x23 is the code segment number (0x20) with RPL set to 3
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs
  movw %ax, %gs

  # Mount a stack like it was created by an interrupt handlers

  movl 0x4(%esp), %eax  # Get the new stack
  movl 0x8(%esp), %ebx   # Get the function

  pushl $0x23      # Code descriptor
  pushl %eax       # Stack
  pushf           # eflags

  pop %eax
  orl $0x200, %eax # Enable interrupts on userland
  pushl %eax

  pushl $0x1B      # Data descriptor
  pushl %ebx       # New EIP

  iret
