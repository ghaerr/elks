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
#include <stdarg.h>

#ifdef __BCC__
#define REGISTER register
#else
#define REGISTER
#endif

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

char *hex_string = "0123456789ABCDEF 0123456789abcdef ";	/* Also used by devices */

static void numout(__u32 v, int width, int base, int useSign,
		   int Lower, int Zero)
{
    __u32 dvr;
    int c, vch;
    register char *i;

    i = (char *)10;
    dvr = 0x3B9ACA00L;
    if (base > 10) {
	i = (char *)8;
	dvr = 0x10000000L;
    }
    if (base < 10) {
	i = (char *)11;
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
	if (c || ((int)i <= width) || ((int)i < 2)) {
	    if ((int)i > width)
		width = (int)i;
	    if (!Zero && !c && ((int)i > 1))
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
    unsigned long v;
    int width, zero;
    char c;
    register char *tmp;

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
	    while ((int)(tmp = (char *)(c - '0')) <= 9) {
		width = width*10 + (int)tmp;
		c = *fmt++;
	    }

	    if ((c == 'h') || (c == 'l'))
		c = *fmt++;
	    tmp = (char *)16;
	    switch (c) {
	    case 'i':
	    case 'd':
		c = 'd'-('X' - 'P');
		tmp = (char *)18;
	    case 'o':
		tmp -= 2;
	    case 'u':
		tmp -= 6;
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
		numout(v, width, (int)tmp, (c == 'd'), (c != 'X'), zero);
		break;
	    case 's':
	    case 't':
		tmp = va_arg(p, char*);
		while ((zero = (int)(c == 's' ? *tmp : (char)get_user_char(tmp)))) {
		    kputchar((char)zero);
		    tmp++;
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
    REGISTER va_list p;

    va_start(p, fmt);
    vprintk(fmt, p);
    va_end(p);
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

    /* Lock up with infinite loop */

    kputs("\n\nSYSTEM LOCKED - Press CTRL-ALT-DEL to reboot:");

    while (1)
	/* Do nothing */;
}
