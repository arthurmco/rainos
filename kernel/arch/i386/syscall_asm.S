#   Syscall asm stub
#
#   Copyright (C) 2017 Arthur M
#

.global _syscalls
_syscalls:  # the syscall "array"
.rept 256
    .long 0
.endr

.global _get_max_syscalls
_get_max_syscalls:
    # Prototype: int get_max_syscalls()
    mov $256, %eax
    ret


.global syscall_stub
syscall_stub:

# On user mode, the registers are like this:
#     syscall_num -> eax
#     p1 -> ebx
#     p2 -> ecx
#     p3 -> edx << 32 | edi
#     ptr -> esi

push %esi
push %edi
push %edx
push %ecx
push %ebx
push %eax

# calculate syscall function in function pointer offset.
mov $_syscalls, %edx
mov %eax, %ebx

# Check if syscalls are in limit
cmp $256, %eax
jg .syscall_bad_num

# Get syscall offset (something like _syscalls + (syscall_num*4))
# Since this code is arch-dependant, i don't worry about portability
# (x86-64, ARM and other might use other registers )
mov 4(%ebx), %eax
add %edx, %eax
call (%eax)

iret

.syscall_bad_num:
    # bad syscall number (> 256)
    mov $0, %eax
    iret
