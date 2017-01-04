#include <arch/i386/specifics.h>

void _memset_8(void* ptr, uint8_t val, size_t num)
{
    asm volatile("rep stosb" :
                      :"c"(num), "D"(ptr), "a"(val));
}
void _memset_16(void* ptr, uint16_t val, size_t num)
{
    asm volatile("rep stosw" :
                      :"c"(num/2), "D"(ptr), "a"(val));
}
void _memset_32(void* ptr, uint32_t val, size_t num)
{
    asm volatile("rep stosl" :
                      :"c"(num/4), "D"(ptr), "a"(val));
}

void _memcpy_8(void* src, void* dst, size_t bytes)
{
    asm volatile("rep movsb" :
                 :"c"(bytes), "D"(dst), "S"(src));
}
void _memcpy_16(void* src, void* dst, size_t bytes)
{
    asm volatile("rep movsw" :
                 :"c"(bytes/2), "D"(dst), "S"(src));
}
void _memcpy_32(void* src, void* dst, size_t bytes)
{
    asm volatile("rep movsl" :
                 :"c"(bytes/4), "D"(dst), "S"(src));
}

void _halt()
{
    asm("hlt");
}
void _cli()
{
    asm("cli");
}
void _sti()
{
    asm("sti");
}
