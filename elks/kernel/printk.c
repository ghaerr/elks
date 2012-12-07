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
 *		%s	string
 *		%t	pointer to string
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

#include <linuxmt/autoconf.h>
#include <arch/segment.h>
#include <linuxmt/mm.h>
#include <stdarg.h>

/*
 *	Just to make it work for now
 */
extern void con_charout(char);

#define kputchar(ch) con_charout(ch)

static void kputs(register char *buf)
{
#ifdef CONFIG_DCON_ANSI_PRINTK

    char *p;

    /* Colourizing */

    static char colour[8] = { 27, '[', '3', '0', ';', '4', '0', 'm' };

    if (++(colour[3]) == '8')
	colour[3] = '1';

    p = colour;
    do
	con_charout(*p);
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

char *hex_string = "0123456789ABCDEF";		/* Also used by devices. */
static char *hex_lower = "0123456789abcdef";

static void numout(unsigned long v, int width, int base, int useSign,
		   int Upper, int Zero)
{
    char *bp, *bp2;
    char buf[12];

    if (width > sizeof(buf))		/* Error-check width specified */
	width = sizeof(buf);

    if (useSign) {
	if ((long)v < 0)
	    v = (-(long)v);
	else
	    useSign = 0;
    }

    bp = buf + sizeof(buf);
    *--bp = '\x00';

	bp2 = Upper ? hex_string : hex_lower;
    do {
	    *--bp = *(bp2 + (v % base));	/* Store digit */
	} while ((v /= base));

    if (useSign && !Zero)
	*--bp = '-';

    width -= buf - bp + sizeof(buf);
    while (--width >= 0)			/* Process width */
	*--bp = Zero ? '0' : ' ';

    if (useSign && Zero && (*bp == '0'))
	*bp = '-';

    kputs(bp);
}

static void vprintk(char *fmt, va_list p)
{
    unsigned long v;
    int width, zero;
    char c, tmp, *cp;

    while ((c = *fmt++)) {
	if (c != '%')
	    kputchar(c);
	else {
	    c = *fmt++;

	    if (c == '%') {
		con_charout(c);
		continue;
	    }

	    width = zero = 0;
	    if (c == '0')
		zero++;
	    while (c >= '0' && c <= '9') {
		width *= 10;
		width += c - '0';
		c = *fmt++;
	    }

	    if ((c == 'h') || (c == 'l'))
		c = *fmt++;
	    tmp = 16;
	    switch (c) {
	    case 'i':
		c = 'd';
	    case 'd':
		tmp = 10;
		if(*(fmt-2) == 'l')
		    v = va_arg(p, unsigned long);
		else
		    v = (long)(va_arg(p, short));
		goto NUMOUT;
	    case 'o':
		tmp -= 2;
	    case 'u':
		tmp -= 6;
	    case 'P':
	    case 'p':
		c += 'X' - 'P';
	    case 'X':
	    case 'x':
		if(*(fmt-2) == 'l')
		    v = va_arg(p, unsigned long);
		else
		    v = (unsigned long)(va_arg(p, unsigned short));
	    NUMOUT:
		numout(v, width, tmp, (c == 'd'), (c == 'X'), zero);
		break;
	    case 's':
		cp = va_arg(p, char*);
		while ((c = *cp++)) {
		    kputchar(c);
		    width--;
		}
		goto FILLSP;
	    case 't':
		cp = va_arg(p, char*);
		while ((c = (char) get_user_char(cp))) {
		    kputchar(c);
		    cp++;
		    width--;
		}
	    FILLSP:
		while (--width >= 0)
		    con_charout(' ');
		break;
	    case 'c':
		while (--width > 0)
		    con_charout(' ');
		kputchar(va_arg(p, int));
		break;
	    default:
		con_charout('?');
		break;
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

void panic(char *error, ...)
{
    va_list p;
    int *bp = (int *) &error - 2, i = 0, j;

    kputs("\npanic: ");
    va_start(p, error);
    vprintk(error, p);
    va_end(p);
    kputs("\napparent call stack:\n");
    kputs("Line: Addr    Parameters\n");
    kputs("~~~~: ~~~~    ~~~~~~~~~~\n");

    do {
	printk("%4u: %04P =>", i, bp[1]);
	bp = (int *) bp[0];
	for (j = 2; j <= 8; j++)
	    printk(" %04X", bp[j]);
	kputchar('\n');
    } while (++i < 9);

    /* Lock up with infinite loop */

    kputs("\nSYSTEM LOCKED - Press CTRL-ALT-DEL to reboot: ");

    while (1)
	/* Do nothing */;
}

void kernel_restarted(void)
{
    panic("kernel restarted\n");
}

void redirect_main(void)
{
    pokeb(get_cs(), 0, 0xe9);
    pokew(get_cs(), 1, ((__u16) kernel_restarted) - 3);
}
