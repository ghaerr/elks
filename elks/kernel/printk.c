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
 *              %o      octal
 *              %s      string in kernel space
 *              %t      string in user space
 *              %T      string w/specified segment
 *              %x/%X   hexadecimal with lower/upper case letters
 *              %#x/%#X hexadecimal using 0x alt prefix
 *              %p      pointer - same as %04x
 *              %D      device name as %04x
 *              %P      process ID
 *              %k      pticks (0.838usec intervals auto displayed as us, ms or s)
 *
 *      All except %% can be followed by a width specifier 1 -> 31 only
 *      and the h/l length specifiers also work where appropriate.
 *
 *              Alan Cox.
 *
 *      MTK:    Sep 97 - Misc hacks to shrink generated code
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
#include <arch/segment.h>
#include <arch/irq.h>
#include <stdarg.h>

#define CONFIG_PREC_TIMER   0   /* =1 to include %k precision timer printk format */

dev_t dev_console;

#ifdef CONFIG_CONSOLE_SERIAL
#define DEVCONSOLE  MKDEV(TTY_MAJOR,RS_MINOR_OFFSET)    /* /dev/ttyS0*/
#else
#define DEVCONSOLE  MKDEV(TTY_MAJOR,TTY_MINOR_OFFSET)   /* /dev/tty1*/
#endif
static void (*kputc)(dev_t, int) = 0;


void set_console(dev_t dev)
{
    register struct tty *ttyp;

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

/************************************************************************
 *
 *      Output a number
 */

static char hex_string[] = "0123456789ABCDEF 0123456789abcdef ";

static void numout(unsigned long v, int width, unsigned int base, int type,
    int Zero, int alt)
{
    unsigned long dvr;
    int c, vch, i;
    int useSign, Lower, Decimal;

    i = 10;
    dvr = 1000000000L;
    if (base > 10) {
        i = 8;
        dvr = 0x10000000L;
    }
    if (base < 10) {
        i = 11;
        dvr = 0x40000000L;
    }

    Decimal = 0;
#if CONFIG_PREC_TIMER
    int Msecs = 0;
    /* display 1/1193182s get_time*() pticks in range 0.838usec through 42.94sec */
    if (type == 'k') {                  /* format works w/limited ranges only */
        Decimal = 3;
        if (v > 51130563UL)             /* = 2^32 / 84 high max range ~42.94s */
            v = 0;                      /* ... displays 0us */
        if (v > 5125259UL) {            /* = 2^32 / 838 */
            v = v * 84UL;
            Decimal = 2;                /* display xx.xx secs */
        } else
            v = v * 838UL;              /* convert to nanosecs w/o 32-bit overflow */
        if (v > 1000000000UL) {         /* display x.xxx secs */
            v /= 1000000UL;             /* divide using _udivsi3 for speed */
        } else if (v > 1000000UL) {
            v /= 1000UL;                /* display xx.xxx msecs */
            Msecs = 1;
        } else {
            Msecs = 2;                  /* display xx.xxx usecs */
        }
    }
#endif

    useSign = (type == 'd');
    if (useSign && ((long)v < 0L))
        v = (-(long)v);
    else
        useSign = 0;

    Lower = (type != 'X');
    if (Lower)
        Lower = 17;
    vch = 0;
    if (alt && base == 16) {
        kputchar('0');
        kputchar('x');
    }
    do {
        c = (int)(v / dvr);
        v %= dvr;
        dvr /= base;
        if (c || (i <= width) || (i < 2)) {
            if (i > width)
                width = i;
            if (!Zero && !c && (i > 1))
                c = 16;
            else {
                Zero = 1;
                if (useSign) {
                    useSign = 0;
                    vch = '-';
                }
            }
            if (vch)
                kputchar(vch);
            vch = *(hex_string + Lower + c);
            if (type == 'k' && i == Decimal) kputchar('.');
        }
    } while (--i);
    kputchar(vch);
#if CONFIG_PREC_TIMER
    if (type == 'k') {
        if (Msecs)
            kputchar(Msecs == 1? 'm': 'u');
        kputchar('s');
    }
#endif
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
            if (c == '#') {
                alt = 1;
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

int dprintk_on = DEBUG_STARTDEF;        /* toggled by debug events*/
#if DEBUG_EVENT
static void (*debug_cbfuncs[3])();  /* debug event callback function*/

void debug_setcallback(int evnum, void (*cbfunc)())
{
    if ((unsigned)evnum < 3)
        debug_cbfuncs[evnum] = cbfunc;
}

void debug_event(int evnum)
{
    if (evnum == 2) {           /* CTRLP toggles dprintk*/
        dprintk_on = !dprintk_on;
        kill_all(SIGURG);
    }
    if (debug_cbfuncs[evnum])
        debug_cbfuncs[evnum]();
}

void dprintk(const char *fmt, ...)
{
    va_list p;

    if (!dprintk_on)
        return;
    va_start(p, fmt);
    vprintk(fmt, p);
    va_end(p);
}
#endif
