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
 *		%p	pointer (printed in hex)
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

#include <linuxmt/fcntl.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>

#include <arch/segment.h>

/*
 *	Just to make it work for now
 */

extern void con_charout();

static void con_write(register char *buf, int len)
{
    register char *p;

#ifdef CONFIG_DCON_ANSI_PRINTK

    /* Colourizing */

    static char colour[8] = { 27, '[', '3', '0', ';', '4', '0', 'm' };

    p = colour;

    if (++(p[3]) == '8')
	p[3] = '1';

    do
	con_charout(*p);
    while (*p++ != 'm');

    /* END Colourizing */

#endif

    p = (char *) len;

    while (p--) {
	if (*buf == '\n')
	    con_charout('\r');
	con_charout(*buf++);
    }
}

/************************************************************************
 *
 *	Output a number
 */

char *hex_string = "0123456789ABCDEF";		/* Also used by devices. */
char *hex_lower  = "0123456789abcdef";

static void numout(char *ptr, int len, int width, int base, int useSign, int Upper)
{
    long int vs;
    unsigned long int v;
    register char *bp, *bp2;
    char buf[32];

    if (width > 31)			/* Error-check width specified */
	width = 31;

    bp = bp2 = buf + 31;

    if (useSign) {
	vs = (len == 2) ? *((short *) ptr) : *((long *) ptr);
	if (vs < 0) {
	    v = - vs;
	    *bp = '-';
	    con_write(bp, 1);
	} else
	    v = vs;
    } else
	v = (len == 2) ? *((unsigned short *) ptr) : *((unsigned long *) ptr);

    *bp = 0;
    do {
	if (Upper)
	    *--bp = hex_string[v % base]; 	/* Store digit */
	else
	    *--bp = hex_lower[v % base]; 	/* Store digit */
    } while ((v /= base) && (bp > buf));

    while (bp2 - bp < width)			/* Process width */
	*--bp = ' ';

    con_write(bp, buf - bp + sizeof(buf) - 1);
}

void printk(register char *fmt,int a1)
{
    register char *p = (char *) &a1;
    int len, width = 0;
    char c, tmp, *cp;

    while ((c = *fmt++)) {
	if (c != '%')
	    con_write(fmt - 1, 1);
	else {
	    c = *fmt++;

	    if (c == '%') {
		con_write("%", 1);
		continue;
	    }

	    while (c >= '0' && c <= '9') {
		width *= 10;
		width += c - '0';
		c = *fmt++;
	    }

	    len = 2;
	    if (c == 'h')
		c = *fmt++;
	    else if (c == 'l') {
		len = 4;
		c = *fmt++;
	    }

	    switch (c) {
	    case 'o':
		tmp = 8;
		goto NUMOUT;
	    case 'i':
		c = 'd';
	    case 'd':
	    case 'u':
		tmp = 10;
		goto NUMOUT;
	    case 'p':
	    case 'x':
	    case 'X':
		tmp = 16;
	    NUMOUT:
		numout(p, len, width, tmp, (c == 'd'), (c == 'X'));
		p += len;
		break;
	    case 's':
		cp = *((char **) p);
		p += sizeof(char *);
		while (*cp) {
		    con_write(cp++, 1);
		    width--;
		}
		while (width-- > 0)
		    con_write(" ", 1);
		break;
	    case 't':
		cp = *((char **) p);
		p += sizeof(char *);
		while ((tmp = (char) get_fs_byte(cp))) {
		    con_charout(tmp);
		    cp++;
		    width--;
		}
		while (width-- > 0)
		    con_write(" ", 1);
		break;
	    case 'c':
		while (width-- > 1)
		    con_write(" ", 1);
		con_write(p, 1);
		p += 2;
		break;
	    default:
		con_write( "?", 1);
		break;
	    }
	}
    }
}

void panic(char *error, int a1 /* VARARGS... */ )
{
    register int *bp = (int *) &error - 2;
    int *bp2 = bp, i, j, k;

    printk("\npanic: ");
    printk(error, a1);
    printk("\napparent call stack:\n");
    printk("Line Address    Parameters\n~~~~ ~~~~~~~    ~~~~~~~~~~\n");

    i = 0;
    do {
	bp2 = (int *) bp[0];
	k = 0;
	for (j = 2; j < 8; j++)
	    k |= bp2[j];
	if (k) {
	    printk("%3u : %4p  =>", i, bp[1]);
	    for (j = 2; j < 8; j++)
		printk(" %4p", bp2[j]);
	    printk("\n");
	} else
	    printk("---\nEND OF STACK\n");
	bp = bp2;
    } while (k && (++i > 9));

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
