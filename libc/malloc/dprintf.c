#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <paths.h>
#include <fcntl.h>

/*
 * Very tiny printf to console.
 * Supports:
 *  %u  unsigned int
 *  %p  unsigned int (displays as unsigned int!)
 *  %d  int (positive numbers only)
 *  %s  char *
 */
int __dprintf(const char *fmt, ...)
{
    unsigned int n;
    char *p;
    va_list va;
    char b[80];
    static int fd = -1;

    if (fd < 0)
        fd = open(_PATH_CONSOLE, O_WRONLY);
    va_start(va, fmt);
    for (n = 0; *fmt; fmt++) {
        if (*fmt == '%') {
            switch (*++fmt) {
            case 's':
                p = va_arg(va, char *);
                goto outstr;
            case 'p':   /* displays as unsigned int! */
            case 'd':
            case 'u':
                p = uitoa(va_arg(va, unsigned int));
        outstr:
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
    return write(fd, b, n);
}
