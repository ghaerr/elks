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

#include <linuxmt/autoconf.h>
#include <arch/segment.h>
#include <linuxmt/mm.h>
#include <stdarg.h>

#define BUFFER_SIZE 12
/*
 *	Just to make it work for now
 */
extern void con_charout(char);

#define kputchar(ch) con_charout(ch)

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

char *hex_string = "0123456789ABCDEF";		/* Also used by devices. */
static char *hex_lower = "0123456789abcdef";

static void numout(unsigned long v, int width, int base, int useSign,
		   int Upper, int Zero)
{
    register char *bp;
    char *bp2;
    char buf[BUFFER_SIZE+1];

    if (width > BUFFER_SIZE)		/* Error-check width specified */
	width = BUFFER_SIZE;

    if (useSign) {
	if ((long)v < 0)
	    v = (-(long)v);
	else
	    useSign = 0;
    }

    bp = buf + BUFFER_SIZE;
    *bp = '\x00';

    bp2 = Upper ? hex_string : hex_lower;
    do {
	*--bp = *(bp2 + (v % base));	/* Store digit */
    } while ((v /= base));

    if (useSign && !Zero)
	*--bp = '-';

    width -= buf - bp + BUFFER_SIZE;
    while (--width >= 0)		/* Process width */
	*--bp = Zero ? '0' : ' ';

    if (useSign && Zero && (*bp == '0'))
	*bp = '-';

    kputs(bp);
}

static void vprintk(register char *fmt, va_list p)
{
    unsigned long v;
    int width, zero;
    char c;
    register char *cp;
    unsigned char tmp;

    while ((c = *fmt++)) {
	if (c != '%')
	    kputchar(c);
	else {
	    c = *fmt++;

	    if (c == '%') {
		kputchar(c);
		continue;
	    }

	    width = zero = 0;
	    if (c == '0')
		zero++;
	    while ((tmp = c - '0') <= 9) {
		width = width*10 + tmp;
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
	    NUMOUT:
		if(*(fmt-2) == 'l')
		    v = va_arg(p, unsigned long);
		else {
		    if(c == 'd')
			v = (long)(va_arg(p, int));
		    else
			v = (unsigned long)(va_arg(p, unsigned int));
		}
		numout(v, width, tmp, (c == 'd'), (c == 'X'), zero);
		break;
	    case 's':
	    case 't':
		cp = va_arg(p, char*);
		while ((tmp = (c == 's' ? *cp : (char)get_user_char(cp)))) {
		    kputchar(tmp);
		    cp++;
		    width--;
		}
	    case 'c':
		while (--width >= 0)
		    kputchar(' ');
		if(c == 'c')
		    kputchar(va_arg(p, int));
		break;
	    default:
		kputchar('?');
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
    register int *bp = (int *) &error - 2;
    int i = 0, j;

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
