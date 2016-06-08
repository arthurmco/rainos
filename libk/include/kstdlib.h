/* stdlib functions for kernel

    Copyright (C) 2016 Arthur M */

#include <stdint.h>
#include <stddef.h>


#ifndef _KSTDLIB_H
#define _KSTDLIB_H

void itoa_s(int32_t i, char* str, int base);
void utoa_s(uint32_t i, char* str, int base);

void sleep(unsigned ms);

void memset(void* ptr, uint8_t val, size_t num);

void _memset_8(void* ptr, uint8_t val, size_t num);
void _memset_16(void* ptr, uint16_t val, size_t num);
void _memset_32(void* ptr, uint32_t val, size_t num);

#endif /* end of include guard: _KSTDLIB_H */
