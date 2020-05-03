/* tinytcap.c */

/* This file contains functions which simulate the termcap functions, but which
 * can only describe the capabilities of the ANSI.SYS and NANSI.SYS drivers on
 * an MS-DOS system or the VT-52 emulator of an Atari-ST.  These functions
 * do *NOT* access a "termcap" database file.
 */

#include "config.h"
#if MSDOS || TOS || MINIX || COHERENT || UNIX

#define CAP(str) CAP2((str)[0], (str)[1])
#define CAP2(a,b) (((a) << 8) + ((b) & 0xff))

#if MSDOS
# define VAL2(v,a)	(a)
# define VAL3(v,a,n)	(nansi ? (n) : (a))
static int	nansi;
#endif

#if TOS
# define VAL2(v,a)	(v)
# define VAL3(v,a,n)	(v)
#endif

#if MINIX || COHERENT || UNIX
# define VAL2(v,a)	(a)
# define VAL3(v,a,n)	(n)
#endif

/*
 * bp    buffer for storing the entry -- ignored
 * name  name of the entry
 */

/*ARGSUSED*/
int tgetent(char *bp, char *name)
{
#if MSDOS
	nansi = strcmp(name, "ansi");
#endif
	return 1;
}

int tgetnum(char *id)
{
	switch (CAP(id))
	{
	  case CAP2('l','i'):	return 25;
	  case CAP2('c','o'):	return 80;
	  case CAP2('s','g'):	return 0;
	  case CAP2('u','g'):	return 0;
	  default:		return -1;
	}
}

int tgetflag(char *id)
{
	switch (CAP(id))
	{
#if !MINIX || COHERENT
	  case CAP2('a','m'):	return 1;
#endif
	  case CAP2('b','s'):	return 1;
	  case CAP2('m','i'):	return 1;
	  default:		return 0;
	}
}

/*
 * bp  pointer to pointer to buffer - ignored
 */

/*ARGSUSED*/
char *tgetstr(char *id, char **bp)
{
	switch (CAP(id))
	{
	  case CAP2('c','e'):	return VAL2("\033K", "\033[K");
	  case CAP2('c','l'):	return VAL2("\033E", "\033[2J");

	  case CAP2('a','l'):	return VAL3("\033L", (char *)0, "\033[L");
	  case CAP2('d','l'):	return VAL3("\033M", (char *)0, "\033[M");

	  case CAP2('c','m'):	return VAL2("\033Y%i%+ %+ ", "\033[%i%d;%dH");
	  case CAP2('d','o'):	return VAL2("\033B", "\033[B");
	  case CAP2('n','d'):	return VAL2("\033C", "\033[C");
	  case CAP2('u','p'):	return VAL2("\033A", "\033[A");
	  case CAP2('t','i'):	return VAL2("\033e", "");
	  case CAP2('t','e'):	return VAL2("", "");

	  case CAP2('s','o'):	return VAL2("\033p", "\033[7m");
	  case CAP2('s','e'):	return VAL2("\033q", "\033[m");
	  case CAP2('u','s'):	return VAL2((char *)0, "\033[4m");
	  case CAP2('u','e'):	return VAL2((char *)0, "\033[m");
	  case CAP2('m','d'):	return VAL2((char *)0, "\033[1m");
	  case CAP2('m','e'):	return VAL2((char *)0, "\033[m");

#if MINIX || COHERENT
	  case CAP2('k','u'):	return "\033[A";
	  case CAP2('k','d'):	return "\033[B";
	  case CAP2('k','l'):	return "\033[D";
	  case CAP2('k','r'):	return "\033[C";
	  case CAP2('k','P'):	return "\033[V";
	  case CAP2('k','N'):	return "\033[U";
	  case CAP2('k','h'):	return "\033[H";
# if MINIX
	  case CAP2('k','H'):	return "\033[Y";
# else /* COHERENT */
	  case CAP2('k','H'):	return "\033[24H";
# endif
#else /* MS-DOS or TOS */
	  case CAP2('k','u'):	return "#H";
	  case CAP2('k','d'):	return "#P";
	  case CAP2('k','l'):	return "#K";
	  case CAP2('k','r'):	return "#M";
	  case CAP2('k','h'):	return "#G";
	  case CAP2('k','H'):	return "#O";
	  case CAP2('k','P'):	return "#I";
	  case CAP2('k','N'):	return "#Q";
#endif

	  default:		return (char *)0;
	}
}

/*
 * cm       cursor movement string -- ignored
 * destcol  destination column, 0 - 79
 * destrow  destination row, 0 - 24
 */

/*ARGSUSED*/
char *tgoto(char *cm, int destcol, int destrow)
{
	static char buf[30];

#if MSDOS || MINIX || COHERENT
	sprintf(buf, "\033[%d;%dH", destrow + 1, destcol + 1);
#endif
#if TOS
	sprintf(buf, "\033Y%c%c", ' ' + destrow, ' ' + destcol);
#endif
	return buf;
}

/*
 * cp      the string to output
 * affcnt  number of affected lines -- ignored
 * outfn   the output function
 */

/*ARGSUSED*/
void tputs(char *cp, int affcnt, int (*outfn)())
{
	while (*cp)
	{
		(*outfn)(*cp);
		cp++;
	}
}
#endif
