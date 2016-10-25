/*  Extended BIOS data area functions

    Copyright (C) 2016 Arthur M.
*/

#include "bda.h"
#include <stddef.h>

#ifndef _EBDA_H
#define _EBDA_H

/* Get the EBDA base address */
uintptr_t ebda_get_base();

/*  Search EBDA for a string, returns the address for it
    Useful to find tables such as RSDT or the MP ones

    Returns the string address, or NULL.
    */
uintptr_t ebda_search_string(const char* str, size_t alignment);


#endif /* end of include guard: _EBDA_H */
