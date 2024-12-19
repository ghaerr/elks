#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <paths.h>
#include <fcntl.h>

static char *uitostr(unsigned int val, int radix, int width)
{
    static char buf[6];
    char *p = buf + sizeof(buf) - 1;
    unsigned int c;

    *p = '\0';
    do {
        c = val % radix;
        val = val / radix;
        if (c > 9)
            *--p = 'a' - 10 + c;
        else
            *--p = '0' + c;
    } while (val);
    c = (radix == 16)? '0': ' ';
    while (buf + sizeof(buf) - 1 - p < width)
        *--p = c;
    return p;
}

/*
 * Very tiny printf to console.
 * Supports:
 *  %{0-9}  width
 *  %u      unsigned decimal
 *  %d      decimal (positive numbers only)
 *  %p      zero-fill hexadecimal width 4
 *  %x      hexadecimal
 *  %s      string
 */
int __dprintf(const char *fmt, ...)
{
    unsigned int n;
    int radix, width;
    char *p;
    va_list va;
    char b[80];
    static int fd = -1;

    if (fd < 0) {
        if (!isatty(STDERR_FILENO))
            fd = STDERR_FILENO;
        else
            fd = open(_PATH_CONSOLE, O_WRONLY);
    }
    va_start(va, fmt);
    for (n = 0; *fmt; fmt++) {
        if (*fmt == '%') {
            ++fmt;
            width = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + *fmt - '0';
                fmt++;
            }
        again:
            switch (*fmt) {
            case 'l':
                fmt++; goto again;
            case 's':
                p = va_arg(va, char *);
                goto outstr;
            case 'p':
                width = 4;
            case 'x':
                radix = 16;
                goto convert;
            case 'd':
            case 'u':
                radix = 10;
        convert:
                p = uitostr(va_arg(va, unsigned int), radix, width);
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
