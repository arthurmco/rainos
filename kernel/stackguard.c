#include <stdint.h>
#include <kstdlog.h>

#define STACK_CHK_GUARD 0x1f2e3d4c;

uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

__attribute__((noreturn))
void __stack_chk_fail(void)
{
    panic("Stack smashing detected!");
}
