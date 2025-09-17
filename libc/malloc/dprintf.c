#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <paths.h>
#include <fcntl.h>

/*
 * Open /dev/console for debug messages. On real hardware, this can
 * be setup using serial console so that the console is /dev/ttyS0,
 * and messages will be seen when Nano-X is running, or use
 * DEBUG_PORT= to specify an alternative device or 'none'.
 *
 * On QEMU, check if /dev/tty1 and /dev/console are the same,
 * and if so open /dev/ttyS0 instead for terminal output assuming
 * QEMU has been run with -serial stdio option which maps /devttyS0 to it.
 */
static int open_debug_terminal(void)
{
    int fd = -1;
    char *debug, *mouse;

    debug = getenv("DEBUG_PORT");
    if (debug) {
        if (!strcmp(debug, "none"))
            return -1;
    } else
        debug = "/dev/ttyS0";       /* preferred console but could be mouse port */

    if (!isatty(STDERR_FILENO))     /* continue piping __dprintf to redirected stderr */
        fd = STDERR_FILENO;
    else {
        mouse = getenv("MOUSE_PORT");
        if (!mouse || strcmp(mouse, debug) != 0)    /* use debug if not used by mouse */
            fd = open(debug, O_NOCTTY | O_WRONLY);
        if (fd < 0)
            fd = open(_PATH_CONSOLE, O_NOCTTY | O_WRONLY);
    }
    return fd;
}

static char *fmtultostr(unsigned long val, int radix, int width)
{
    static char buf[34];
    char *p = buf + sizeof(buf) - 1;
    unsigned int c;

    *p = '\0';
    do {
#ifdef _M_I86
        c = radix;
        val = __divmod(val, &c);
#else
        c = val % radix;
        val = val / radix;
#endif
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
 *  %{0-9}  field width
 *  %l{u,d,x,p}
 *  %u      unsigned decimal
 *  %d      decimal (positive numbers only)
 *  %p      pointer (zero-filled hexadecimal)
 *  %x      hexadecimal
 *  %s      string
 */
int __dprintf(const char *fmt, ...)
{
    unsigned int n;
    unsigned long val;
    int radix, width;
    char *p;
    va_list va;
    int lval;
    char b[128];
    static int fd = -1;

    if (fd < 0)
        fd = open_debug_terminal();
    if (fd < 0) return 0;
    va_start(va, fmt);
    for (n = 0; *fmt; fmt++) {
        if (*fmt == '%') {
            ++fmt;
            width = 0;
            lval = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + *fmt - '0';
                fmt++;
            }
        again:
            switch (*fmt) {
            case 'l':
                lval = 1;
                fmt++;
                goto again;
            case 's':
                p = va_arg(va, char *);
                goto outstr;
            case 'p':
                if (sizeof(char *) == sizeof(unsigned long))
                    lval = 1;
                width = lval? 8: 4;
                /* fall through */
            case 'x':
                radix = 16;
                goto convert;
            case 'd':
            case 'u':
                radix = 10;
        convert:
                val = lval? va_arg(va, unsigned long): va_arg(va, unsigned int);
                p = fmtultostr(val, radix, width);
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
