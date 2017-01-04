#include <arch/i386/tss.h>

extern gdt_tss_t dTSS[];
extern volatile uintptr_t stack_top[];

static volatile tss_t tss;

/*  Initialize the TSS struct.
    Make the GDT TSS part point to the TSS */
void tss_init(uintptr_t cr3)
{
    dTSS[0].base_low = ((uintptr_t)&tss) & 0xffffff;
    dTSS[0].base_high = (((uintptr_t)&tss) >> 24) & 0xff;
    dTSS[0].limit_low = sizeof(tss_t);
    dTSS[0].limit_high = 0;

    tss.ss0 = 0x10; /* The segment used for stack */
    tss.esp0 = (uint32_t)stack_top;
    tss.cr3 = cr3;

    tss_flush();
}


/*  Use the 'ltr' instruction to load the TSS
    into the processor */
void tss_flush()
{
    /*  The TSS stays on the 5th position on the GDT, and the
        requested privilege level is 3 (because if it wasn't,
        this TSS couldn't execute user mode code) */

    asm("ltr %%ax" : : "a"(0x2b));
}
