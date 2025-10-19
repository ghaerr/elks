/*
 *      This is NOT stunningly portable, but works for pretty dumb non-ANSI
 *      compilers and is tight. Adjust the sizes to taste.
 *
 *      Illegal format strings will break it. Only the following simple
 *      printf() subset is supported:
 *
 *              %%      literal % sign
 *              %c      char
 *              %d/%i   signed decimal
 *              %u      unsigned decimal
 *              %,d/,u  thousands separator
 *              %o      octal
 *              %s      string in kernel space
 *              %t      string in user space
 *              %T      string w/specified segment
 *              %x/%X   hexadecimal with lower/upper case letters
 *              %#x/%#X hexadecimal using 0x alt prefix
 *              %p      pointer - same as %04x
 *              %D      device name as %04x
 *              %E      device name as /dev/...
 *              %P      process ID
 *              %k      pticks (0.838usec intervals auto displayed as us, ms or s)
 *              %#k     pticks truncated at decimal point
 *
 *      All except %% can be followed by a width specifier 1 -> 31 only
 *      and the h/l length specifiers also work where appropriate.
 *
 *              Alan Cox.
 *
 *      MTK:    Sep 97 - Misc hacks to shrink generated code
 *      GRH:    Sep 24 - Rewritten for speed
 */

#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/major.h>
#include <linuxmt/ntty.h>
#include <linuxmt/debug.h>
#include <linuxmt/signal.h>
#include <linuxmt/prectimer.h>
#include <arch/segment.h>
#include <arch/irq.h>
#include <arch/divmod.h>
#include <stdarg.h>

#define CONFIG_PREC_TIMER   1   /* =1 to include %k precision timer printk format */

dev_t dev_console;

#ifdef CONFIG_CONSOLE_SERIAL
#define DEVCONSOLE  MKDEV(TTY_MAJOR,RS_MINOR_OFFSET)    /* /dev/ttyS0*/
#else
#define DEVCONSOLE  MKDEV(TTY_MAJOR,TTY_MINOR_OFFSET)   /* /dev/tty1*/
#endif
static void (*kputc)(dev_t, int) = 0;


void set_console(dev_t dev)
{
    struct tty *ttyp;

    if (dev == 0)
            dev = DEVCONSOLE;
    if ((ttyp = determine_tty(dev))) {
            kputc = ttyp->ops->conout;
            dev_console = dev;
    }
}

void kputchar(int ch)
{
    if (ch == '\n')
            kputchar('\r');
    if (kputc)
            (*kputc)(dev_console, ch);
    else early_putchar(ch);
}

static void kputs(const char *buf)
{
    while (*buf)
        kputchar(*buf++);
}

/* convert 1/1193182s get_ptime() pticks (0.838usec through 42.85sec) for display */
static unsigned long conv_ptick(unsigned long v, int *pDecimal, int *pSuffix)
{
#ifdef CONFIG_PREC_TIMER
    unsigned int c;
    int Suffix = 0;

    /* format works w/limited ranges only */
    if (v > 5125259UL) {            /* = 2^32 / 838  = ~4.3s */
        if (v > 51130563UL)         /* = 2^32 / 84 high max range = ~42.85s */
            v = 0;                  /* ... displays 0us */
        v *= 84;
        *pDecimal = 2;              /* display xx.xx secs */
    } else {
        v *= 838;                   /* convert to nanosecs w/o 32-bit overflow */
        *pDecimal = 3;              /* display xx.xxx */
    }
    while (v > 1000000L) {          /* quick divides for readability */
        c = 1000;
        v = __divmod(v, &c);
        Suffix++;
    }
    if (Suffix == 0)      *pSuffix = ('s' << 8) | 'u';
    else if (Suffix == 1) *pSuffix = ('s' << 8) | 'm';
    else                  *pSuffix = 's';
#endif
    return v;
}

/************************************************************************
 *
 *      Output a number
 */

static void numout(unsigned long v, int width, unsigned int base, int type,
    int Zero, int alt)
{
    int n, i;
    unsigned int c;
    char *p;
    int Sign, Suffix, Decimal;
    char buf[12];                       /* small stack: good up to max long octal v */

    Decimal = -1;
    Sign = Suffix = 0;

    if (type == 'd'  && (long)v < 0L) {
        v = -(long)v;
        Sign = 1;
    }

    if (alt && base == 16) {
        kputchar('0');
        kputchar('x');
    }

    if (type == 'k')
        v = conv_ptick(v, &Decimal, &Suffix);

    p = buf + sizeof(buf) - 1;
    *p = '\0';
    for (i = 0;;) {
        c = base;
        v = __divmod(v, &c);                /* remainder returned in c */
        if (c > 9)
            *--p = ((type == 'X')? 'A': 'a') - 10 + c;
        else
            *--p = '0' + c;
        if (!v)
            break;
        if ((alt == ',' || alt == '\'') && ++i == 3) {
            *--p = alt;
            i = 0;
        }
    }
    n = buf + sizeof(buf) - 1 - p + Sign;   /* string length */
    if (n > width)                          /* expand field width */
        width = n;
    if (Zero && Sign) {
        kputchar('-');
        Sign = 0;
    }
    for (i = n; i++ < width; )
        kputchar(Zero? '0': ' ');
    if (Sign)
        kputchar('-');
    while (*p) {
        if (n-- == Decimal) {               /* only for %k pticks */
            if (alt)
                break;
            kputchar('.');
        }
        kputchar(*p++);
    }
    while (Suffix) {
        kputchar(Suffix & 255);
        Suffix >>= 8;
    }
}

static void vprintk(const char *fmt, va_list p)
{
    int c, n, width, zero, alt, ptrfmt;
    unsigned long v;
    char *str;

    while ((c = *fmt++)) {
        if (c != '%')
            kputchar(c);
        else {
            c = *fmt++;

            if (c == '%') {
                kputchar(c);
                continue;
            }

            ptrfmt = alt = width = 0;
            if (c == '#' || c == ',' || c == '\'') {
                alt = c;
                c = *fmt++;
            }
            zero = (c == '0');
            while ((n = (c - '0')) <= 9) {
                width = width*10 + n;
                c = *fmt++;
            }

            if ((c == 'h') || (c == 'l'))
                c = *fmt++;
            n = 16;
            switch (c) {
            case 'i':
            case 'd':
                c = 'd';
                n = 18;
            case 'o':
                n -= 2;
            case 'u':
            case 'k':
                n -= 6;
            case 'X':
            case 'x':
            hex:
                if (*(fmt-2) == 'l') {
                    if (ptrfmt) width = 8;
                    v = va_arg(p, unsigned long);
                } else {
                    if (c == 'd')
                        v = (long)(va_arg(p, int));
                    else
                        v = (unsigned long)(va_arg(p, unsigned int));
                }
            out:
                numout(v, width, n, c, zero, alt);
                break;
            case 'P':
                v = current->pid;
                c = 'd';
                n = 10;
                goto out;
            case 'D':
                c += 'X' - 'D';
            case 'p':
                c += 'X' - 'P';
                ptrfmt = 1;
                zero = 1;
                width = 4;
                goto hex;
            case 'T':
                n = va_arg(p, unsigned int);
                goto str;
            case 't':
                n = current->t_regs.ds;
                goto str;
            case 'E':
                kputs(root_dev_name(va_arg(p, unsigned int))+8); /* skip ROOTDEV= */
                break;
            case 's':
                n = kernel_ds;
            str:
                str = va_arg(p, char *);
                while ((zero = peekb((word_t)str++, n))) {
                    kputchar(zero);
                    width--;
                }
            case 'c':
                while (--width >= 0)
                    kputchar(' ');
                if (c == 'c')
                    kputchar(va_arg(p, int));
                break;
            default:
                kputchar('?');
            }
        }
    }
}

void printk(const char *fmt, ...)
{
    va_list p;

    va_start(p, fmt);
    vprintk(fmt, p);
    va_end(p);
}

void halt(void)
{
    /* Lock up with infinite loop */
    kputs("\nSYSTEM HALTED - Press CTRL-ALT-DEL to reboot:");

    /*clr_irq();*/  /* uncomment to halt interrupt handlers, but then CAD won't work */
    while (1)
        idle_halt();
}

void panic(const char *error, ...)
{
    va_list p;

    kputs("\npanic: ");
    va_start(p, error);
    vprintk(error, p);
    va_end(p);

#if UNUSED      // FIXME rewrite for ia16 call stack
    int *bp = (int *) &error - 2;
    char *j;
    int i = 0;
    kputs("\napparent call stack:\n"
          "Line: Addr    Parameters\n"
          "~~~~: ~~~~    ~~~~~~~~~~");

    do {
        printk("\n%4u: %04P =>", i, bp[1]);
        bp = (int *) bp[0];
        j = (char *)2;
        do {
            printk(" %04X", bp[(int)j]);
        } while ((int)(++j) <= 8);
    } while (++i < 9);
#endif

    halt();
}

int debug_level = DEBUG_LEVEL;      /* set with debug= or toggled by debug events */
#if DEBUG_EVENT
static void (*debug_cbfuncs[3])();  /* debug event callback function*/

void debug_setcallback(int evnum, void (*cbfunc)())
{
    if ((unsigned)evnum < 3)
        debug_cbfuncs[evnum] = cbfunc;
}

void debug_event(int evnum)
{
    if (evnum == 2) {               /* CTRLP toggles debug */
        debug_level = !debug_level;
        kill_all(SIGURG);
    }
    if (debug_cbfuncs[evnum])
        debug_cbfuncs[evnum]();
}

void dprintk(const char *fmt, ...)
{
    va_list p;

    if (!debug_level)
        return;
    va_start(p, fmt);
    vprintk(fmt, p);
    va_end(p);
}
#endif
