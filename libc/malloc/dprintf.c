#if VERBOSE
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

int __debug_level = 1;

/*
 * Very tiny printf to stderr.
 * Supports:
 *  %u  unsigned int
 */
int __dprintf(const char *fmt, ...)
{
    unsigned int n = 0;
    char *p;
    va_list va;
    char b[128];

    if (__debug_level == 0)
        return 0;
    va_start(va, fmt);
    for (; *fmt; fmt++) {
        if (*fmt == '%') {
            switch (*++fmt) {
            case 'u':
                p = uitoa(va_arg(va, unsigned int));
                while (*p && n < sizeof(b))
                    b[n++] = *p++;
                break;
            }
            continue;
        } else {
            if (n < sizeof(b))
                b[n++] = *fmt;
        }
    }
    va_end(va);
    return write(2, b, n);
}
#endif
