#include "task.h"

static struct task_list tasklist;
static size_t taskID = 0;

static struct task_list_item* task_curr_item = NULL;
static int switched = 0;

void task_irq_handler(regs_t* regs) {
    if (!switched) return;

    /*  Save this register set into the old task structure and
        put the registers from the next task */

    /* Emulate a circular list */
    struct task_list_item* old = task_curr_item;
    struct task_list_item* new = old->next ? old->next : tasklist.first;

    if (!new) {
        panic("Task list corruption! 'next' task shouldn't be NULL");
    }

    /* Same task? Return. */
    /* TODO: Go idle */
    //if (old == new) return;

    old->item->regs.eax = regs->eax;
    old->item->regs.ebx = regs->ebx;
    old->item->regs.ecx = regs->ecx;
    old->item->regs.edx = regs->edx;
    old->item->regs.esi = regs->esi;
    old->item->regs.edi = regs->edi;
    old->item->regs.esp = regs->old_esp;
    old->item->regs.ebp = regs->ebp;
    old->item->regs.eip = regs->eip;
    old->item->regs.eflags = regs->eflags;
    asm volatile("mov %%cr3, %%eax" : "=a"(old->item->regs.cr3));

    knotice("\teax: %08x\t ebx: %08x\t ecx: %08x\t edx:%08x", regs->eax, regs->ebx, regs->ecx, regs->edx);
    knotice("\tesi: %08x\t edi: %08x\t esp: %08x\t ebp:%08x", regs->esi, regs->edi, regs->old_esp, regs->ebp);
    knotice("\teip: %08x\t eflags: %08x\t cr3: %08x", regs->eip, regs->eflags, old->item->regs.cr3);

    knotice("\n----");

    regs->eax = new->item->regs.eax;
    regs->ebx = new->item->regs.ebx;
    regs->ecx = new->item->regs.ecx;
    regs->edx = new->item->regs.edx;
    regs->esi = new->item->regs.esi;
    regs->edi = new->item->regs.edi;
    regs->old_esp = new->item->regs.esp;
    regs->ebp = new->item->regs.ebp;
    regs->eip = new->item->regs.eip;
    regs->eflags = new->item->regs.eflags;
    asm volatile("mov %%eax, %%cr3" : : "a"(new->item->regs.cr3));

    knotice("\teax: %08x\t ebx: %08x\t ecx: %08x\t edx:%08x", regs->eax, regs->ebx, regs->ecx, regs->edx);
    knotice("\tesi: %08x\t edi: %08x\t esp: %08x\t ebp:%08x", regs->esi, regs->edi, regs->old_esp, regs->ebp);
    knotice("\teip: %08x\t eflags: %08x\t cr3: %08x", regs->eip, regs->eflags, new->item->regs.cr3);

    task_curr_item = new;
    switched = 0;

    /*  Return. The iret in the final of this irq handler will
        take care of returning to the right place */
}

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

    irq_add_handler(0, &task_irq_handler);

    knotice("task: Task system initialised. Kernel task is %x", task_curr_item);
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
    tli->next = tasklist.first;
    tasklist.first->prev = tli;

    tasklist.last = tli;
    tasklist.count++;

    task_t* tsk = kcalloc(sizeof(task_t), 1);
    tsk->id = (taskID++);

    /* TODO: move specific register initialization out of here */
    memset(&tsk->regs, 0, sizeof(task_regs_t));
    tsk->regs.eip = pc;
    tsk->regs.cr3 = pagedir;
    tsk->regs.eflags = pflags;
    tsk->regs.esp = vmm_alloc_page(VMM_AREA_KERNEL, 1) + 0xf00;
    tsk->regs.ebp = tsk->regs.esp;

    knotice("task: Task created, id %x, new stack at %08x",
        tsk->id, tsk->regs.esp-0xff0);
    tli->item = tsk;

    return tsk;
}


void task_remove(task_t* task)
{

}

void task_switch()
{
    asm volatile("cli");

    /* Emulate a circular list */
    struct task_list_item* old = task_curr_item;
    struct task_list_item* new = old->next ? old->next : tasklist.first;

    knotice("from task %d [%x, %x] (pc %x) to task %d [%x %x] (pc %x)",
        old->item->id, old->item, old, old->item->regs.eip,
        new->item->id, new->item, new, new->item->regs.eip);

    switched = 1;

    asm("sti; hlt");
}


/* Returns the current task */
task_t* task_get_current()
{
    return task_curr_item->item;
}
