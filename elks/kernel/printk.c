/*
 *	This is NOT stunningly portable, but works for pretty dumb non-ANSI
 *	compilers and is tight. Adjust the sizes to taste.
 *
 *	Illegal format strings will break it. Only the following simple
 *	printf() subset is supported:
 *
 *		%%	literal % sign
 *
 *		%c	char
 *		%d	signed decimal
 *		%i	signed decimal
 *		%o	octal
 *		%p/%P	pointer - same as %x/%X respectively
 *		%s	string in kernel space
 *		%t	string in user space
 *		%u	unsigned decimal
 *		%x/%X	hexadecimal with lower/upper case letters
 *
 *	All except %% can be followed by a width specifier 1 -> 31 only
 *	and the h/l length specifiers also work where appropriate.
 *
 *		Alan Cox.
 *
 *	MTK:	Sep 97 - Misc hacks to shrink generated code
 */

#include <linuxmt/config.h>
#include <arch/segment.h>
#include <linuxmt/mm.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/major.h>
#include <linuxmt/ntty.h>
#include <linuxmt/debug.h>
#include <linuxmt/signal.h>
#include <stdarg.h>

dev_t dev_console;

#ifdef CONFIG_CONSOLE_SERIAL
#define DEVCONSOLE  MKDEV(TTY_MAJOR,RS_MINOR_OFFSET)	/* /dev/ttyS0*/
#else
#define DEVCONSOLE  MKDEV(TTY_MAJOR,TTY_MINOR_OFFSET)	/* /dev/tty1*/
#endif
static void (*kputc)(dev_t, char) = 0;


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

static void kputs(register char *buf)
{
#ifdef CONFIG_EMUL_ANSI_PRINTK

    char *p;

    /* Colourizing */

    static char colour[8] = { 27, '[', '3', '0', ';', '4', '0', 'm' };

    if (++(colour[3]) == '8')
	colour[3] = '1';

    p = colour;
    do
	kputchar(*p);
    while (*p++ != 'm');

    /* END Colourizing */

#endif

    while (*buf)
	kputchar(*buf++);
}

/************************************************************************
 *
 *	Output a number
 */

char *hex_string = "0123456789ABCDEF 0123456789abcdef ";	/* Also used by devices */

static void numout(__u32 v, int width, int base, int useSign,
		   int Lower, int Zero)
{
    __u32 dvr;
    int c, vch, i;

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

    if (useSign && ((long)v < 0L))
	v = (-(long)v);
    else
	useSign = 0;

    if (Lower)
	Lower = 17;
    vch = 0;
    do {
	c = (int)(v / dvr);
	v %= dvr;
	dvr /= (unsigned int)base;
	if (c || (i <= width) || (i < 2)) {
	    if (i > width)
		width = (int)i;
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
	}
    } while (--i);
    kputchar(vch);
}

static void vprintk(register char *fmt, va_list p)
{
    int c, n, width, zero;
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

	    width = 0;
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
		c = 'd'-('X' - 'P');
		n = 18;
	    case 'o':
		n -= 2;
	    case 'u':
		n -= 6;
	    case 'P':
	    case 'p':
		c += 'X' - 'P';
	    case 'X':
	    case 'x':
		if (*(fmt-2) == 'l')
		    v = va_arg(p, unsigned long);
		else {
		    if (c == 'd')
			v = (long)(va_arg(p, int));
		    else
			v = (unsigned long)(va_arg(p, unsigned int));
		}
		numout(v, width, n, (c == 'd'), (c != 'X'), zero);
		break;
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

void printk(char *fmt, ...)
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
	/* Do nothing */;
}

void panic(char *error, ...)
{
    va_list p;
    register int *bp = (int *) &error - 2;
    register char *j;
    int i = 0;

    kputs("\npanic: ");
    va_start(p, error);
    vprintk(error, p);
    va_end(p);

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

    halt();
}

int dprintk_on = DEBUG_STARTDEF;	/* toggled by debug events*/
#if DEBUG_EVENT
static void (*dprintk_cb)();		/* debug event callback function*/

void debug_setcallback(void (*cbfunc))
{
    dprintk_cb = cbfunc;
}

void debug_event(void)
{
    dprintk_on = !dprintk_on;
    if (dprintk_cb)
	dprintk_cb();
    kill_all(SIGURG);
}

void dprintk(char *fmt, ...)
{
    va_list p;

    if (!dprintk_on)
	return;
    va_start(p, fmt);
    vprintk(fmt, p);
    va_end(p);
}
#endif
