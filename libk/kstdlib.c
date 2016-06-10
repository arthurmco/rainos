#include <kstdlib.h>
#include <kstring.h>

#include "../kernel/kheap.h"
#include "../kernel/arch/i386/devices/pit.h"

static const char num2str[] = "0123456789ABCDEFGHIJ\0\0\0";
void itoa_s(int32_t i, char* str, int base)
{
    int index = 0;
    char strb[sizeof(i)*8];
    int neg = i < 0;
    i = neg ? ((i ^ 0xffffffff)+1) : i;
    do {
        uint32_t num = i % base;
        strb[index++] = num2str[num];
        i /= base;
    } while (i > 0);

    if (neg) {
        strb[index++] = '-';
    }
    strb[index] = 0;

    strrev(strb, str);
}


void utoa_s(uint32_t i, char* str, int base) {
    int index = 0;
    char strb[sizeof(i)*8];
    int32_t num;
    do {
        num = i % base;
        strb[index++] = num2str[num];
        i /= base;
    } while (i > 0);

    strb[index] = 0;
    strrev(strb, str);
}

void memset(void* ptr, uint8_t val, size_t num)
{
    /*if ((num % 4) == 0)
        _memset_32(ptr, val, num);
    else*/ if ((num % 2) == 0)
        _memset_16(ptr, val, num);
    else
        _memset_8(ptr, val, num);
}

void _memset_8(void* ptr, uint8_t val, size_t num)
{
    asm volatile("rep stosb" :
                      :"c"(num), "D"(ptr), "aN"(val));
}
void _memset_16(void* ptr, uint16_t val, size_t num)
{
    asm volatile("rep stosw" :
                      :"c"(num/2), "D"(ptr), "aN"(val));
}
/*
void _memset_32(void* ptr, uint32_t val, size_t num)
{
    asm volatile("rep stosl" :
                      :"C"(num/4), "D"(ptr), "N"(val));
} */

void sleep(unsigned ms)
{
    uint64_t ctr_begin = pit_get_counter();
    uint64_t ctr = ctr_begin;

    while (ctr < (ctr_begin+ms)) {
        asm("nop");
        ctr = pit_get_counter();
    }

}

void memcpy(void* src, void* dst, size_t bytes)
{
    asm volatile("rep movsb" :
                 :"c"(bytes), "D"(dst), "S"(src));
}

void* kmalloc(size_t bytes)
{
    return kheap_allocate(bytes);
}
void* kcalloc(size_t bytes, size_t count)
{
    void* m = kheap_allocate(bytes*count);
    for (int i = 0; i < count; i++)
        memset(m, 0, bytes);
}
void kfree(void* addr)
{
    kheap_deallocate(addr);
}
