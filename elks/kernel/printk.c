/*
 *	This is NOT stunningly portable, but works
 *	for pretty dumb non ANSI compilers and is
 *	tight.	Adjust the sizes to taste.
 *
 *	Illegal format strings will break it. Only the
 *	following simple subset is supported
 *
 *	%%	-	literal % sign
 *	%c	-	char
 *	%d	-	signed decimal
 *	%o	-	octal
 *	%p	-	pointer (printed in hex)
 *	%s	-	string
 *	%u	-	unsigned decimal
 *	%x/%X	-	hexadecimal
 *
 *	And the h/l length specifiers for %d/%x
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
#ifdef CONFIG_DCON_ANSI_PRINTK
    /* Colourizing */

    static char colour[8] = { 27, '[', '3', '1', ';', '4', '0', 'm' };

    {
	register char *p = colour;

	if (p[3] == '8') {
	    p[3] = '1';
	}

	do {
	    con_charout(*p);
	    if (*p == 'm')
		break;
	    ++p;
	} while (1);

	++p[-4];
    }

    /* END Colourizing */
#endif

    {
	register char *p = (char *) len;

	while (p) {
	    if (*buf == '\n')
		con_charout('\r');
	    con_charout(*buf);
	    ++buf;
	    --p;
	}
    }
}

/************************************************************************
 *
 *	Output a number
 */

char *hex_string = "0123456789ABCDEF";		/* Also used by devices. */

static void numout(char *ptr, int len, int base, int useSign)
{
    long int vs;
    unsigned long int v;
    register char *bp;
    char buf[16];

    bp = buf + 15;

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
	*--bp = hex_string[(v % base)]; 	/* Store digit. */
    } while ((v /= base) && (bp > buf));

    con_write(bp, buf - bp + sizeof(buf) - 1);
}

void printk(register char *fmt,int a1)
{
    register char *p = (char *) &a1;
    int len;
    char c, tmp;

    while ((c = *fmt++)) {
	if (c != '%')
	    con_write(fmt - 1, 1);
	else {
	    len = 2;

	    c = *fmt++;
	    if (c == 'h')
		c = *fmt++;
	    else if (c == 'l') {
		len = 4;
		c = *fmt++;
	    }

	    switch (c) {
	    case 'd':
	    case 'u':
		tmp = 10;
		goto NUMOUT;
	    case 'o':
		tmp = 8;
		goto NUMOUT;
	    case 'p':
	    case 'x':
	    case 'X':
		tmp = 16;
	    NUMOUT:
		numout(p, len, tmp, (c == 'd'));
		p += len;
		break;
	    case 's':
		{
		    char *cp = *((char **) p);
		    p += sizeof(char *);
		    while (*cp)
			con_write(cp++, 1);
		    break;
		}
	    case 't':
		{
		    char *cp = *((char **) p);
		    p += sizeof(char *);
		    while ((tmp = (char) get_fs_byte(cp))) {
			con_charout(tmp);
			cp++;
		    }
		    break;
		}
	    case 'c':
		con_write(p, 1);
		p += 2;
		break;
	    default:
		con_write( (c == '%') ? "%" : "?", 1);
	    }
	}
    }
}

void panic(char *error, int a1 /* VARARGS... */ )
{
#if 1
    register int *bp = (int *) &error - 2;
    register char *j;
    int i;

    printk("\npanic: ");
    printk(error, a1);

    printk("\napparent call stack:\n");
    for (i = 0; i < 8; i++) {
	printk("(%u) return address = %p, params = ", i, bp[1]);
	bp = (int *) bp[0];
	for (j = (char *) 2; ((int) j) < 8; j++)
	    printk(" %p", bp[(int) j]);
	printk("\n");
    }
    while (1);
#else
    register int *bp = (int *) &error - 2;
    int i, j;

    printk("\npanic: ");
    printk(error, a1);

    printk("\napparent call stack:\n");
    for (i = 0; i < 8; i++) {
	printk("(%u) return address = %p, params = ", i, bp[1]);
	bp = (int *) bp[0];
	for (j = 2; j < 8; j++)
	    printk(" %p", bp[j]);
	printk("\n");
    }
    while (1);
#endif
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
