#include <arch/i386/syscall.h>
#include <kstdlog.h>

/* asm stub for syscall */
extern void syscall_stub();

/*  asm-allocated functions.
    The syscalls will be called inside the stub. */
extern volatile syscall_func _syscalls[];
extern volatile int _get_max_syscalls();

/* kprintf test.
    ptr-> string to be printed.

    returns 0
    */
static int syscall_kprintf_test(int n, int p1, int p2, uint64_t p3, uintptr_t ptr)
{
    knotice("syscall: n=%d, p1=0x%08x, p2=0x%08x, p3=(%x_%x), ptr=0x%08x",
        n, p1, p2, (unsigned int)(p3 >> 32), (unsigned int)(p3 & 0xffffffff),
        ptr);

    if (!ptr) return -1;

    char* s = (char*)ptr;
    kprintf(s);

    return 1;
}


/* Create syscall vector. Return 1 on success, 0 on error. */
int syscall_init()
{
    idt_addentry(SYSCALL_INT, 0x08, &syscall_stub, IDTA_INTERRUPT | IDTA_USERMODE);
    knotice("syscall: added syscall handler to vector %x", SYSCALL_INT);
    syscall_add(1, &syscall_kprintf_test);
}

/* Add syscall vector. Return 1 on success, 0 if the vector already exists */
int syscall_add(int num, syscall_func f)
{
    if (num > _get_max_syscalls()) {
        kerror("syscall: tried to add syscall #%d, but maximum syscall num is #%d",
            num, _get_max_syscalls());
        return 0;
    }

    if (_syscalls[num]) {
        kerror("syscall: syscall #%d already has a function", num);
        return 0;
    }

    _syscalls[num] = f;
    return 1;
}

/* Remove syscall vector. Return 1 on success, 0 if vector doesn't exist */
int syscall_remove(int num, syscall_func f)
{
    if (num > _get_max_syscalls()) {
        kerror("syscall: tried to remove syscall #%d, but maximum syscall is %d",
            num, _get_max_syscalls());
        return 0;
    }

    if (!_syscalls[num]) {
        kerror("syscall: tried to remove unexistent syscall #%d", num);
        return 0;
    }

    if (_syscalls[num] != f) {
        kerror("syscall: tried to remove mismatched syscall #%d", num);
        return 0;
    }

    _syscalls[num] = NULL;
    return 1;
}
