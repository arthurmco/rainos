/*
    Processor specific code for task handling

    Copyright (C) 2016 Arthur M
*/

#include <stdint.h>

#include "vmm.h"

#ifndef TASK_H
#define TASK_H

typedef struct {
    uint32_t eip, eflags, cr3;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;    /* pushed by pusha instruction */
} task_regs_t;

extern void task_true_switch(task_regs_t* old, task_regs_t* new);


#endif /* end of include guard: TASK_H */
