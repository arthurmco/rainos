#include <kstdlib.h>
#include <kstring.h>

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
