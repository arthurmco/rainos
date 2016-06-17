#include <kstdlib.h>
#include <kstring.h>

#include "../kernel/kheap.h"
#include "../kernel/arch/i386/devices/pit.h"

static const char num2str[] = "0123456789ABCDEFGHIJ\0\0\0";

int atoi(const char* str, int base)
{
    size_t len = strlen(str);
    char revstr[len];
    strrev(str, revstr);
    int num = 0;
    unsigned int mul = 1;

    for (size_t i = 0; i < len; i++)
    {
        size_t basenum = 0;
        if (revstr[i] >= '0' && revstr[i] <= '9') {
            basenum = revstr[i] - '0';
        } else if (revstr[i] >= 'A' && revstr[i] <= 'Z') {
            basenum = revstr[i] - 'A' + 10;
        } else if (revstr[i] >= 'a' && revstr[i] <= 'z') {
            basenum = revstr[i] - 'a' + 10;
        }

        num += (basenum * mul);
        mul *= base;

        if (revstr[i] == '-')
        {
            num = -num;
            break; // This is always the last char in the number
        }

        if (revstr[i] == 'x'){
            break; //start of 0x
        }
    }

    return num;
}

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

void utoa_s_pad(uint32_t i, char* str, int base, int pad, char padchar)
{
    int index = 0;
    char strb[sizeof(i)*8];
    int32_t num;
    int len = 0;

    do {
        num = i % base;
        strb[index++] = num2str[num];
        i /= base;
        len++;
    } while (i > 0);

    while (len < pad) {
        strb[index++] = padchar;
        len++;
    }

    strb[index] = 0;
    strrev(strb, str);
}

void memset(void* ptr, uint8_t val, size_t num)
{
    if ((num % 4) == 0)
        _memset_32(ptr, val, num);
    else if ((num % 2) == 0)
        _memset_16(ptr, val, num);
    else
        _memset_8(ptr, val, num);
}

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
    if ((bytes % 4) == 0)
        _memcpy_32(src, dst, bytes);
    else if ((bytes % 2) == 0)
        _memcpy_16(src, dst, bytes);
    else
        _memcpy_8(src, dst, bytes);
}

void* kmalloc(size_t bytes)
{
    return (void*)kheap_allocate(bytes);
}
void* kcalloc(size_t bytes, size_t count)
{
    void* m = kheap_allocate(bytes*count);
    for (size_t i = 0; i < count; i++)
        memset(m, 0, bytes);

    return m;
}
void kfree(void* addr)
{
    kheap_deallocate((virtaddr_t)addr);
}
