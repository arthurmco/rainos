#include "ebda.h"
#include <kstring.h>

/* Get the EBDA base address */
uintptr_t ebda_get_base()
{
    uintptr_t base = bda_ptr->ebda_base;
    return (base << 4);
}

/*  Search EBDA for a string, returns the address for it
    Useful to find tables such as RSDP or the MP ones */
uintptr_t ebda_search_string(const char* str, size_t alignment)
{
    uintptr_t start = ebda_get_base();
    int len = strlen(str);
    for (uintptr_t addr = start; addr < 0x100000; addr += alignment)
    {
        char* addrstr = (char*)addr;
        if (!strncmp(addrstr, str, len-1)) {
            return addr;
        }
    }

    return NULL;
}
