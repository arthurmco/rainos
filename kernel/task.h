/*  Generic tasking code

    Copyright (C) 2016 Arthur M */

#include "arch/i386/task.h"

#include <kstdlog.h>
#include <kstdlib.h>

#ifndef _GEN_TASK_H
#define _GEN_TASK_H

typedef struct _task {
    uint32_t id;
    task_regs_t regs;
} task_t;

struct task_list_item {
    task_t *item;
    struct task_list_item *prev, *next;
};

struct task_list {
    struct task_list_item* first;
    struct task_list_item* last;
    size_t count;
};

/* Init the tasking subsystem */
void task_init();

/* Creates a task.
    pc is the new program counter (aka EIP on x86 processors)
*/
task_t* task_create(uint32_t pc, uint32_t pagedir, uint32_t pflags);
void task_remove(task_t* task);

void task_switch();

/* Returns the current task */
task_t* task_get_current();



#endif /* end of include guard: _GEN_TASK_H */
