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

#include <linuxmt/types.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/sched.h>

/*
 *	Just to make it work for now
 */
 
extern void con_charout();
 
static void con_write(buf,len)
register char *buf;
int len;
{
	char c;

#ifdef CONFIG_DCON_ANSI_PRINTK

	/* Colourizing */
	static char colour= '1';
	
	if (colour=='8') colour='1';
		 	
	con_charout(27);
	con_charout('[');
	con_charout('3');
	con_charout(colour);
	con_charout(';');
	con_charout('4');
	con_charout('0');
	con_charout('m');

	colour++;
	/* END Colourizing */

#endif
	
	while (len) {
		c = *buf;
		if (c == '\n')
			con_charout('\r');	
		con_charout(c);
		len--;
		buf++;
	}
}
 
/************************************************************************/

/*
 *	Output a number
 */

static char *nstring="0123456789ABCDEF";
 
static void numout(ptr,len,base,useSign)
unsigned long *ptr;
int len;
int base;
int useSign;
{
	char buf[16];	/* Largest value must fit in this */
	register char *bp=buf+14;
	unsigned long v;
	int c;

	bp[1]=0;
	v=*ptr;
	if(len == 2)
		v = (unsigned short)v;
	if(useSign && v < 0) {
		con_write("-",1);
		v=-v;
	}

	do {
		c=v%base;			/* This digit */
		*bp--=nstring[c];		/* String for it */
		v/=base;			/* Slide along */
	} while(v);
	
	bp++;
	con_write(bp,buf-bp+sizeof(buf)-1);
}

void printk(fmt,a1)
register char *fmt;
int a1;
{
	register unsigned char *p=(char *)&a1;
	char c, tmp;

	while(c=*fmt++) {
		if(c!='%')
			con_write(fmt-1,1);
		else {
			int len=2;
			c=*fmt++;
			if(c=='h')
				c=*fmt++;
			else if(c=='l') {
				len=4;
				c=*fmt++;
			}
			
			switch(c)				
			{
				case 'd':
				case 'u':
					numout(p,len,10,(c=='d'));
					p+=len;
					break;
				case 'o':
				case 'p':
				case 'x':
				case 'X':
					numout(p,len,(c=='o')?8:16,0);
					p+=len;
					break;
				case 's':
				{
					char *cp=*((char **)p);
					p+=sizeof(char*);
					while(*cp)
						con_write(cp++,1);
					break;
				}
				case 't':
				{
					char * cp=*((char **)p);
					p+=sizeof(char *);
					while ((tmp = get_fs_byte(cp))) {
						con_charout(tmp);
						cp++;
					}
					break;
				}
				case 'c':
					con_write(p,1);
					p+=2;
					break;
				case '%':
					con_write("%",1);
					break;
				default:
					con_write("?",1);
			}
		}
	}
}

void panic(error,a1,a2,a3,a4)
char *error;
int a1,a2,a3,a4;	/* VARARGS... */
{
	register int *bp = (int*)&error - 2;
	int i, j;

	printk("\npanic: ");
	printk(error,a1,a2,a3,a4);

	printk("\napparant call stack:\n");
	for (i = 0; i < 8; i++) {
		printk("(%u) ret addr = %p params = ", i, bp[1]);		
		bp = (int*)bp[0];
		for (j = 2; j < 8; j++) {
			printk(" %p", bp[j]);
		}
		printk("\n");
	}
	while(1);
}

void kernel_restarted() {
	panic("kernel restarted\n");
}

void redirect_main() {
	pokeb(get_cs(), 0, 0xe9);
	pokew(get_cs(), 1, (int)kernel_restarted - 3);
}
