#include <kstdlog.h>
#include <kstring.h>

terminal_t* logterm = NULL;
void klog_set_device(terminal_t* term)
{
    logterm = term;
}

void knotice(const char* fmt, ...)
{
    char msg[255];

    va_list vl;
    va_start(vl, fmt);
    vsprintf(msg, fmt, vl);
    va_end(vl);

    strcat(msg, "\n");

    if (logterm->term_puts_f)
        logterm->term_puts_f(msg);
}

void kwarn(const char* fmt, ...)
{
    char msg[255];

    if (logterm->term_setcolor_f) {
        /* Print WARNING in yellow foreground */
        logterm->term_setcolor_f(0x0e);
        if (logterm->term_puts_f)
            logterm->term_puts_f("WARNING: ");
        logterm->term_setcolor_f(logterm->defaultColor);
    }

    va_list vl;
    va_start(vl, fmt);
    vsprintf(msg, fmt, vl);
    va_end(vl);

    strcat(msg, "\n");

    if (logterm->term_puts_f)
        logterm->term_puts_f(msg);
}

void kerror(const char* fmt, ...)
{
    char msg[255];

    if (logterm->term_setcolor_f) {
        /* Print ERROR in red foreground */
        logterm->term_setcolor_f(0x0c);
        if (logterm->term_puts_f)
            logterm->term_puts_f("ERROR: ");
        logterm->term_setcolor_f(logterm->defaultColor);
    }

    va_list vl;
    va_start(vl, fmt);
    vsprintf(msg, fmt, vl);
    va_end(vl);

    strcat(msg, "\n");

    if (logterm->term_puts_f)
        logterm->term_puts_f(msg);

    /* Print to standard output now */
    terminal_setcolor(0x0c);
    terminal_puts("ERROR: ");
    terminal_restorecolor();
    terminal_puts(msg);
}

void panic(const char* fmt, ...)
{
    /* Get some of the registers */
    int eax,ebx,ecx,edx,esi,edi;
    asm("nop": "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx), "=S"(esi), "=D"(edi));
    
    char msg[255];

    if (logterm->term_setcolor_f) {
        /* Print ERROR in red foreground */
        logterm->term_setcolor_f(0x0c);
        if (logterm->term_puts_f)
            logterm->term_puts_f("\nPANIC: ");
        logterm->term_setcolor_f(logterm->defaultColor);
    }

    va_list vl;
    va_start(vl, fmt);
    vsprintf(msg, fmt, vl);
    va_end(vl);

    strcat(msg, "\n");

    if (logterm->term_puts_f)
        logterm->term_puts_f(msg);

    /* Print to standard output now */
    terminal_setcolor(0x0c);
    terminal_puts("\nPANIC: ");
    terminal_restorecolor();
    terminal_puts(msg);

    kprintf("\n\teax: 0x%x, ebx: 0x%x, ecx: 0x%x, edx: 0x%x\n\t"
        "esi: 0x%x, edi: 0x%x\n System halted.",
        eax, ebx, ecx, edx, esi, edi);
    asm("cli; hlt");
}
