#include "task.h"

static struct task_list tasklist;
static size_t taskID = 0;

static struct task_list_item* task_curr_item;

/* Init the tasking subsystem */
void task_init()
{
    tasklist.count = 0;
    tasklist.first = NULL;
    tasklist.last = NULL;

    /* Get the actual pagedir and eflags and create a task representing us */
    uint32_t pagedir, eflags;
    asm volatile("movl %%cr3, %%eax; movl %%eax, %0;":"=m"(pagedir)::"%eax");
    asm volatile("pushfl; movl (%%esp), %%eax; movl %%eax, %0; popfl;":"=m"(eflags)::"%eax");

    task_t* tKernel = task_create((uint32_t)&task_init, pagedir, eflags);
    task_curr_item = tasklist.first;

    knotice("task: Task system initialised");
}

/* Creates a task.
    pc is the new program counter (aka EIP on x86 processors)
*/
task_t* task_create(uint32_t pc, uint32_t pagedir, uint32_t pflags)
{
    knotice("task: Creating task, pc:%08x, pagedir:%08x physical, flags: %08x",
        pc, pagedir, pflags);

    /* Adds the new task at the end of the task list */
    struct task_list_item* tli = (struct task_list_item*)
        kcalloc(sizeof(struct task_list_item), 1);
    tli->prev = tasklist.last;
    if (!tasklist.first)
        tasklist.first = tli;

    tli->prev->next = tli;
    tli->next = NULL;
    tasklist.last = tli;
    tasklist.count++;

    task_t* tsk = kcalloc(sizeof(task_t), 1);
    tsk->id = (taskID++);

    /* TODO: move specific register initialization out of here */
    memset(&tsk->regs, 0, sizeof(task_regs_t));
    tsk->regs.eip = pc;
    tsk->regs.cr3 = pagedir;
    tsk->regs.eflags = pflags;
    tsk->regs.esp = vmm_alloc_page(VMM_AREA_KERNEL, 1) + 0x1000;

    knotice("task: Task created, id %x, new stack at %08x",
        tsk->id, tsk->regs.esp-0x1000);

    return tsk;
}

void task_remove(task_t* task)
{

}

void task_switch()
{
    /* Emulate a circular list */
    struct task_list_item* old = task_curr_item;
    struct task_list_item* new = old->next ? old->next : tasklist.first;

    task_true_switch(&old->item->regs, &new->item->regs);
}

/* Returns the current task */
task_t* task_get_current()
{
    return task_curr_item->item;
}
