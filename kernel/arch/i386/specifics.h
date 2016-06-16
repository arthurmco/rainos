/*  Specific assembly code for x86

    Copyright (C) 2016 Arthur M

*/

#include <stdint.h>
#include <stddef.h>

#ifndef _SPECIFICS_H
#define _SPECIFICS_H

#define RAIN_X86 1

void _memset_8(void* ptr, uint8_t val, size_t num);
void _memset_16(void* ptr, uint16_t val, size_t num);
void _memset_32(void* ptr, uint32_t val, size_t num);

void _memcpy_8(void* src, void* dst, size_t bytes);
void _memcpy_16(void* src, void* dst, size_t bytes);
void _memcpy_32(void* src, void* dst, size_t bytes);

void _halt();
void _cli();
void _sti();



#endif /* end of include guard: _SPECIFICS_H */
