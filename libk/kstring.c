#include <kstring.h>
#include <kstdlib.h>

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

void sprintf(char* str, const char* fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    vsprintf(str, fmt, vl);
    va_end(vl);
}

void vsprintf(char* str, const char* fmt, va_list vl) {
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            unsigned padding = 0;
            char padchar = ' ';
            char padstr[8];
            int padstrptr = 0;
            /* detect padding specifiers */
            while ( (fmt[padstrptr] >= '0' &&
                     fmt[padstrptr] <= '9') ) {
                    if (padstrptr == 0 && fmt[padstrptr] == '0')
                    {
                        padchar = '0';
                        fmt++;
                        continue;
                    }
                    padstr[padstrptr] = fmt[padstrptr];
                    padstrptr++;

            }

            if (padstrptr > 0) {
                padstr[padstrptr] = 0;
                padding = atoi(padstr, 10);
                fmt += padstrptr;
            }

            //tokens
            switch (*fmt) {
                //literal %
                case '%':
                    *str = '%';
                    break;

                //signed integer
                case 'd': {
                    char num[16];
                    int32_t i = va_arg(vl, int32_t);
                    itoa_s(i, num, 10);
                    *str = 0;
                    str = strcat(str, num);
                    str++;

                }
                    break;

                //hex
                case 'x': {
                    char num[8];
                    uint32_t i = va_arg(vl, uint32_t);
                    if (padding > 0)
                        utoa_s_pad(i, num, 16, padding, padchar);
                    else
                        utoa_s(i, num, 16);
                    *str = 0;
                    str = strcat(str, num);
                    str++;
                }
                    break;

                //string
                case 's': {
                    char* s = va_arg(vl, char*);
                    *str = 0;
                    str = strcat(str, s);
                    str++;

                }
                    break;

                //char
                case 'c': {
                    char c = va_arg(vl, int);
                    *str = c;
                    str++;
                }
                    break;
            }

        } else {
            *str = *fmt;
            str++;
        }
        fmt++;
    }
    *str = 0;
}

char* strcat(char* str, const char* catted)
{
    size_t len = strlen(str);
    char* cstart = &str[len];

    while (*catted) {
        *cstart = *catted;
        cstart++;
        catted++;
    }

    *cstart = 0;

    return --cstart;
}
