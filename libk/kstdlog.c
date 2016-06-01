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
