/*  Register definitions for the i386 architeture

    Copyright (C) 2016 Arthur M.
*/

#include <stdint.h>

#ifndef _REGS_H
#define _REGS_H

/*  Common registers, in the order pushed by isr_common
    on idt_asm.S */
typedef struct _regs_i386 {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} regs_t;


#endif /* end of include guard: _REGS_H */
