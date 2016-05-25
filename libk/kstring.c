#include <kstring.h>

void strrev(const char* str, char* rev) {
    size_t len = strlen(str);

    for (size_t i = 0; i < len; i++) {
        rev[i] = str[len-i-1];
    }
    rev[len] = 0;
}

size_t strlen(const char* s) {
    size_t i = 0;
    while (*s) {
        s++;
        i++;
    }

    return i;
}
