/*  Usermode jumping function

    Copyright (C) 2016 Arthur M.
    */

#    C prototype:
#     void jump_usermode(uintptr_t stack, void (*usermode_function)()) */
.global jump_usermode
.type jump_usermode, @function
.align 16
jump_usermode:
  cli

  mov $0x23, %ax   # 0x23 is the data segment number (0x20) with RPL set to 3
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs

  # Mount a stack like it was created by an interrupt handlers

  movl 0x4(%esp), %eax  # Get the new stack
  movl 0x8(%esp), %ebx   # Get the function

  pushl $0xdeadcafe

  pushl $0x23      # Data descriptor
  pushl %eax       # Stack

  pushfl           # eflags
  popl %eax
  orl $0x200, %eax # Enable interrupts on userland
  pushl %eax

  pushl $0x1B      # Code descriptor
  pushl %ebx       # New EIP

  iret
  ret
