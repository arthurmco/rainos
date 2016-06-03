/* stdlib functions for kernel

    Copyright (C) 2016 Arthur M */

#include <stdint.h>


#ifndef _KSTDLIB_H
#define _KSTDLIB_H

void itoa_s(int32_t i, char* str, int base);
void utoa_s(uint32_t i, char* str, int base);

void sleep(unsigned ms);



#endif /* end of include guard: _KSTDLIB_H */
