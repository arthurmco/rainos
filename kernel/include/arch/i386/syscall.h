/*  Syscall setup and definition

    Copyright (C) 2017 Arthur M
*/

#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <stdint.h>
#include <stdarg.h> //for va_list
#include <arch/i386/idt.h>

/*  Syscall interrupt number.
    Use the same as Linux, might ease things.
 */
#define SYSCALL_INT 0x80

/*  Syscall function pointer.
    Obligatory, the syscall has 5 params: one for the number,
    2 integer parameters, 1 64-bit parameter and 1 pointer parameter.

    On user mode, the registers are like this:
        syscall_num -> eax
        p1 -> ebx
        p2 -> ecx
        p3 -> edx << 32 | edi
        ptr -> esi

    This might suffice most of cases.

    The syscall returns 1 on success or a negative value for error
    (usually -errno )

    TODO: declare and use errno in kernel.
 */
typedef int (*syscall_func)(int syscall_num, int p1, int p2,
    uint64_t p3, uintptr_t ptr);

/* Create syscall vector. Return 1 on success, 0 on error. */
int syscall_init();

/* Add syscall vector. Return 1 on success, 0 if the vector already exists */
int syscall_add(int num, syscall_func f);

/* Remove syscall vector. Return 1 on success, 0 if vector doesn't exist */
int syscall_remove(int num, syscall_func f);



#endif /* end of include guard: _SYSCALL_H */
