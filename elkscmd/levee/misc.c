/*
 * LEVEE, or Captain Video;  A vi clone
 *
 * Copyright (c) 1982-1997 David L Parsons
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by David L Parsons (orc@pell.chi.il.us).  My name may not be used
 * to endorse or promote products derived from this software without
 * specific prior written permission.  THIS SOFTWARE IS PROVIDED
 * AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "levee.h"
#include "extern.h"

bool PROC
getline(str)
 char *str;
{
    int len;
    char flag;
    
    flag = line(str, 0, COLS-curpos.x, &len);
    str[len] = 0;
    strput(CE);
    return (flag == EOL);
} /* getline */


char PROC
readchar()
{
    ch = peekc();		/* get the peeked character */
    needchar = TRUE;		/* force a read on next readchar/peekc */
    if (xerox) {		/* save this character for redo */
	if (rcp >= &rcb[256-1])	/* oops, buffer overflow */
	    error();
	else			/* concat it at the end of rcb^ */
	    *rcp++ = ch;
    }
    return ch;
} /* readchar */


/* look at next input character without actually using it */
char PROC
peekc()
{
    if (needchar) {				/* if buffer is empty, */
	if (macro >= 0) {			/* if a macro */
	    lastchar = *mcr[macro].ip;
	    mcr[macro].ip++;
	    if (*mcr[macro].ip == 0) {
		if (--mcr[macro].m_iter > 0)
		    mcr[macro].ip = mcr[macro].mtext;
		else
		    --macro;
	    }
	}
	else				/* else get one from the keyboard */
	    lastchar = getKey();
        needchar = FALSE;
    }
    return lastchar;
} /* peekc */


/* find the amount of leading whitespace between start && limit.
   endd is the last bit of whitespace found.
*/
int PROC
findDLE(start, endd, limit, dle)
int start, limit, dle;
 int *endd;
{
    while ((core[start] == '\t' || core[start] == ' ') && start < limit) {
	if (core[start] == '\t')
	    dle = tabsize * (1+(dle/tabsize));
	else
	    dle++;
	start++;
    }
    *endd = start;
    return dle;
} /* findDLE */


int PROC
skipws(loc)
int loc;
{
    while ((core[loc] == '\t' || core[loc] == ' ') && loc <= bufmax)
	loc++;
    return(loc);
} /* skipws */


int PROC
setX(cp)
int cp;
{
    int top, xp;
    
    top = bseekeol(cp);
    xp = 0;
    while (top < cp) {
	switch (cclass(core[top])) {
	    case 0 : xp++; break;
	    case 1 : xp += 2; break;
	    case 2 : xp = tabsize*(1+(xp/tabsize)); break;
	    case 3 : xp += 3; break;
	}
	top++;
    }
    return(xp);
} /* setX */


int PROC
setY(cp)
int cp;
{
    int yp, ix;
    
    ix = ptop;
    yp = -1;
    cp = min(cp,bufmax-1);
    do {
	yp++;
	ix = 1+fseekeol(ix);
    } while (ix <= cp);
    return(yp);
} /* setY */


int PROC
to_line(cp)
int cp;
{
    int tdx,line;
    tdx = 0;
    line = 0;
    while (tdx <= cp) {
	tdx = 1+fseekeol(tdx);
	line++;
    }
    return(line);
} /* to_line */


int PROC
to_index(line)
int line;
{
    int cp = 0;
    while (cp < bufmax && line > 1) {
	cp = 1+fseekeol(cp);
	line--;
    }
    return(cp);
} /* to_index */
    
PROC
swap(a,b)
 register int *a,*b;
{
    int c;
    
    c = *a;
    *a = *b;
    *b = c;
} /* swap */


int PROC
#if ST
cclass(c)
register int c;
{
    if (c >= ' ' && c < '')
	return 0;
    if (c == '\t' && !list)
	return 2;
    if (c >= 0)
	return 1;
    return 3;
} /* cclass */
#else
cclass(c)
register unsigned char c;
{
    if (c == '\t' && !list)
	return 2;
    if (c == '' || c < ' ')
	return 1;
#if MSDOS==0
    if (c & 0x80)
	return 3;
#endif
    return 0;
} /* cclass */
#endif

#if ST
/*
 * wildly machine-dependent code to make a beep
 */
#include <atari\osbind.h>

static char sound[] = {
	0xA8,0x01,0xA9,0x01,0xAA,0x01,0x00,
	0xF8,0x10,0x10,0x10,0x00,0x20,0x03
};

#define SADDR	0xFF8800L

typedef char srdef[4];

static srdef *SOUND = (srdef *)SADDR;

static
beeper()
{
    register i;
    for (i=0; i<sizeof(sound); i++) {
	(*SOUND)[0] = i;
	(*SOUND)[2] = sound[i];
    }
} /* beeper */
#endif /*ST*/


PROC
error()
{
    indirect = FALSE;
    macro = -1;
    if (xerox)
	rcb[0] = 0;
    xerox = FALSE;
#if ST
    Supexec(beeper);
#else
    if (bell)
	strput(BELL);
#endif /*ST*/
} /* error */


/* the dirty work to start up a macro */
PROC
insertmacro(cmdstr, count)
 register char *cmdstr;
int count;
{
    if (macro >= NMACROS)
	error();
    else if (*cmdstr != 0) {
	macro++;
	mcr[macro].mtext = cmdstr;	/* point at the text */
	mcr[macro].ip = cmdstr;		/* starting index */
	mcr[macro].m_iter = count;	/* # times to do the macro */
    }
} /* insertmacro */


int PROC
lookup(c)
char c;
{
    int ix = MAXMACROS;
    
    while (--ix >= 0 && mbuffer[ix].token != c)
	;
    return ix;
} /* lookup */


PROC
fixmarkers(base,offset)
int base,offset;
{
    char c;
    
    for (c = 0;c<'z'-'`';c++)
	if (contexts[c] > base)
	    if (contexts[c]+offset < base || contexts[c]+offset >= bufmax)
		contexts[c] = -1;
	    else
		contexts[c] += offset;
} /* fixmarkers */


PROC
wr_stat()
{
    clrprompt();
    if (filenm[0] != 0) {
	printch('"');
	prints(filenm);
	prints("\" ");
	if (newfile)
	    prints("<New file> ");
    }
    else
	prints("No file");
    printch(' ');
    if (readonly)
	prints("<readonly> ");
    else if (modified)
	prints("<Modified> ");
    if (bufmax > 0) {
	prints(" line ");
	printi(to_line(curr));
	prints(" -");
	printi((int)((long)(curr*100L)/(long)bufmax));
	prints("%-");
    }
    else
	prints("-empty-");
} /* wr_stat */


static int  tabptr,
	    tabstack[20],
	    ixp;
	
PROC
back_up(c)
char c;
{
    switch (cclass(c)) {
	case 0: ixp--; break;
	case 1: ixp -= 2; break;
	case 2: ixp = tabstack[--tabptr]; break;
	case 3: ixp -= 3; break;
    }
    mvcur(-1,ixp);
} /* back_up */


/*
 *  put input into buf[] || instring[].
 *  return states are:
 *	0 : backed over beginning
 *    ESC : ended with an ESC
 *    EOL : ended with an '\r'
 */
char PROC
line(s, start, endd, size)
 register char *s;
int start, endd;
 register int *size;
{
    int col0,
	ip;
    char c;
    
    col0 = ixp = curpos.x;
    ip = start;
    tabptr = 0;
    while (1) {
	c = readchar();
	if (movemap[c] == INSMACRO)	/* map!ped macro */
	    insertmacro(mbuffer[lookup(c)].m_text, 1);
	else if (c == DW) {
	    while (!wc(s[ip-1]) && ip > start)
		back_up(s[--ip]);
	    while (wc(s[ip-1]) && ip > start)
		back_up(s[--ip]);
	}
	else if (c == eraseline) {
	    ip = start;
	    tabptr = 0;
	    mvcur(-1,ixp=col0);
	}
	else if (c==erasechar) {
	    if (ip>start)
		back_up(s[--ip]);
	    else {
		*size = 0;
		return(0);
	    }
	}
	else if (c=='\n' || c=='\r' || c==ESC) {
	    *size = (ip-start);
            return (c==ESC) ? ESC : EOL;
	}
	else if ((!beautify) || c == TAB || c == ''
			     || (c >= ' ' && c <= '~')) {
	    if (ip < endd) {
		if (c == '')
		    c = readchar();
		switch (cclass(c)) {
		    case 0 : ixp++; break;
		    case 1 : ixp += 2; break;
		    case 2 :
			tabstack[tabptr++] = ixp;
			ixp = tabsize*(1+(ixp/tabsize));
			break;
		    case 3 : ixp += 3; break;
		}
		s[ip++] = c;
		printch(c);
	    }
	    else
		error();
	}
        else
	    error();
    }
} /* line */


/* move to core[loc] */
PROC
setpos(loc)
int loc;
{
    lstart = bseekeol(loc);
    lend = fseekeol(loc);
    xp = setX(loc);
    curr = loc;
} /* setpos */


PROC
resetX()
{
    if (deranged) {
	xp = setX(curr);
	mvcur(-1, xp);
	deranged = FALSE;
    }
} /* resetX */


/* set end of window */
PROC
setend()
{
    int bottom, count;
    
    bottom = ptop;
    count = LINES-1;
    while (bottom < bufmax && count > 0) {
	bottom = 1+fseekeol(bottom);
	count--;
    }
    pend = bottom-1;		/* last char before eol || eof */
} /* setend */


/*  set top of window
 *  return the number of lines actually between curr && ptop.
 */
int PROC
settop(lines)
int lines;
{
    int top, yp;
    
    top = curr;
    yp = -1;
    do {
	yp++;
	top = bseekeol(top) - 1;
	lines--;
    } while (top >= 0 && lines > 0);
    ptop = top+1;			/* tah-dah */
    setend();
    return(yp);
} /* settop */
