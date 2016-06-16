/* stdlib functions for kernel

    Copyright (C) 2016 Arthur M */

#include <stdint.h>
#include <stddef.h>

#include "../../kernel/arch/i386/specifics.h"

#ifndef _KSTDLIB_H
#define _KSTDLIB_H

int atoi(const char* str, int base);
void itoa_s(int32_t i, char* str, int base);
void utoa_s(uint32_t i, char* str, int base);
void utoa_s_pad(uint32_t i, char* str, int base, int pad, char padchar);

void sleep(unsigned ms);

void memset(void* ptr, uint8_t val, size_t num);

void memcpy(void* src, void* dst, size_t bytes);

void* kmalloc(size_t bytes);
void* kcalloc(size_t bytes, size_t count);
void kfree(void* addr);

#endif /* end of include guard: _KSTDLIB_H */
