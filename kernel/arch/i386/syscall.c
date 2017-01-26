#include <arch/i386/syscall.h>
#include <kstdlog.h>

/* asm stub for syscall */
extern void syscall_stub();

/*  asm-allocated functions.
    The syscalls will be called inside the stub. */
extern syscall_func* _syscalls;
extern int _get_max_syscalls();

/* Create syscall vector. Return 1 on success, 0 on error. */
int syscall_init()
{
    idt_addentry(SYSCALL_INT, 0x18, &syscall_stub, IDTA_INTERRUPT | IDTA_USERMODE);
    knotice("syscall: added syscall handler to vector %x", SYSCALL_INT);
}

/* Add syscall vector. Return 1 on success, 0 if the vector already exists */
int syscall_add(int num, syscall_func f)
{
    if (num < _get_max_syscalls()) {
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
    if (num < _get_max_syscalls()) {
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
