/* pc.c */

/* Author:
 *	Guntram Blohm
 *	Buchenstrasse 19
 *	7904 Erbach, West Germany
 *	Tel. ++49-7305-6997
 *	sorry - no regular network connection
 */

/* This file implements the ibm pc bios interface. See IBM documentation
 * for details.
 * If TERM is set upon invocation of elvis, this code is ignored completely,
 * and the standard termcap functions are used, thus, even not-so-close
 * compatibles can run elvis. For close compatibles however, bios output
 * is much faster (and permits reverse scrolling, adding and deleting lines,
 * and much more ansi.sys isn't capable of). GB.
 */

#include "config.h"
#include "vi.h"

#if MSDOS

#include <dos.h>

static void video();

/* vmode contains the screen attribute index and is set by attrset.*/

int vmode;

/* The following array contains attribute definitions for
 * color/monochrome attributes. Screen selects one of the sets.
 * Maybe i'll put them into elvis options one day.
 */

static int screen;
static char attr[2][5] =
{
	/*	:se:	:so:	:VB:	:ul:	:as:	*/
	{	0x1f,	0x1d,	0x1e,	0x1a,	0x1c,	},	/* color */
	{	0x07,	0x70,	0x0f,	0x01,	0x0f,	},	/* monochrome */
};

/*
 * bios interface functions for elvis - pc version
 */

/* cursor up: determine current position, decrement row, set position */

void v_up()
{
	int dx;
	video(0x300,(int *)0,&dx);
	dx-=0x100;
	video(0x200,(int *)0,&dx);
}

#ifndef NO_CURSORSHAPE
/* cursor big: set begin scan to end scan - 4 */
void v_cb()
{
	int cx;
	video(0x300, &cx, (int *)0);
	cx=((cx&0xff)|(((cx&0xff)-4)<<8));
	video(0x100, &cx, (int *)0);
}

/* cursor small: set begin scan to end scan - 1 */
void v_cs()
{
	int cx;
	video(0x300, &cx, (int *)0);
	cx=((cx&0xff)|(((cx&0xff)-1)<<8));
	video(0x100, &cx, (int *)0);
}
#endif

/* clear to end: get cursor position and emit the aproppriate number
 * of spaces, without moving cursor.
 */
 
void v_ce()
{
	int cx, dx;
	video(0x300,(int *)0,&dx);
	cx=COLS-(dx&0xff);
	video(0x920,&cx,(int *)0);
}

/* clear screen: clear all and set cursor home */

void v_cl()
{
	int cx=0, dx=((LINES-1)<<8)+COLS-1;
	video(0x0600,&cx,&dx);
	dx=0;
	video(0x0200,&cx,&dx);
}

/* clear to bottom: get position, clear to eol, clear next line to end */

void v_cd()
{
	int cx, dx, dxtmp;
	video(0x0300,(int *)0,&dx);
	dxtmp=(dx&0xff00)|(COLS-1);
	cx=dx;
	video(0x0600,&cx,&dxtmp);
	cx=(dx&0xff00)+0x100;
	dx=((LINES-1)<<8)+COLS-1;
	video(0x600,&cx,&dx);
}

/* add line: scroll rest of screen down */

void v_al()
{
	int cx,dx;
	video(0x0300,(int *)0,&dx);
	cx=(dx&0xff00);
	dx=((LINES-1)<<8)+COLS-1;
	video(0x701,&cx,&dx);
}

/* delete line: scroll rest up */

void v_dl()
{
	int cx,dx;
	video(0x0300,(int *)0,&dx);
	cx=(dx&0xff00)/*+0x100*/;
	dx=((LINES-1)<<8)+COLS-1;
	video(0x601,&cx,&dx);
}

/* scroll reverse: scroll whole screen */

void v_sr()
{
	int cx=0, dx=((LINES-1)<<8)+COLS-1;
	video(0x0701,&cx,&dx);
}

/* set cursor */

void v_move(x,y)
	int x, y;
{
	int dx=(y<<8)+x;
	video(0x200,(int *)0,&dx);
}

/* put character: set attribute first, then execute char.
 * Also remember if current line has changed.
 */

int v_put(ch)
	int ch;
{
	int cx=1;
	ch&=0xff;
	if (ch>=' ')
		video(0x900|ch,&cx,(int *)0);
	video(0xe00|ch,(int *)0, (int *)0);
	if (ch=='\n')
	{	exwrote = TRUE;
		video(0xe0d, (int *)0, (int *)0);
	}
	return ch;
}

/* determine number of screen columns. Also set attrset according
 * to monochrome/color screen.
 */

int v_cols()
{
	union REGS regs;
	regs.h.ah=0x0f;
	int86(0x10, &regs, &regs);
	if (regs.h.al==7)			/* monochrome mode ? */
		screen=1;
	else
		screen=0;
	return regs.h.ah;
}

/* Getting the number of rows is hard. Most screens support 25 only,
 * EGA/VGA also support 43/50 lines, and some OEM's even more.
 * Unfortunately, there is no standard bios variable for the number
 * of lines, and the bios screen memory size is always rounded up
 * to 0x1000. So, we'll really have to cheat.
 * When using the screen memory size, keep in mind that each character
 * byte has an associated attribute byte.
 *
 * uses:	word at 40:4c contains	memory size
 *		byte at 40:84 		# of rows-1 (sometimes)
 *		byte at	40:4a		# of columns
 */

int v_rows()
{
	int line, oldline;

	/* screen size less then 4K? then we have 25 lines only */

	if (*(int far *)(0x0040004cl)<=4096)
		return 25;

	/* VEGA vga uses the bios byte at 0x40:0x84 for # of rows.
	 * Use that byte, if it makes sense together with memory size.
	 */

	if ((((*(unsigned char far *)(0x0040004aL)*2*
		(*(unsigned char far *)(0x00400084L)+1))+0xfff)&(~0xfff))==
		*(unsigned int far *)(0x0040004cL))
			return *(unsigned char far *)(0x00400084L)+1;

	/* uh oh. Emit '\n's until screen starts scrolling. */

	v_move(oldline=0, 0);
	for (;;)
	{
		video(0xe0a,(int *)0,(int *)0);
		video(0x300,(int *)0,&line);
		line>>=8;
		if (oldline==line)
			return line+1;
		oldline=line;	
	}
}

/* the REAL bios interface -- used internally only. */

static void video(ax, cx, dx)
	int ax, *cx, *dx;
{
	union REGS regs;

	regs.x.ax=ax;
	if ((ax&0xff00)==0x600 || (ax&0xff00)==0x700)
		regs.h.bh=attr[screen][vmode];
	else
	{
		regs.h.bh=0;
		regs.h.bl=attr[screen][vmode];
	}
	if (cx) regs.x.cx=*cx;
	if (dx) regs.x.dx=*dx;
	int86(0x10, &regs, &regs);
	if (dx) *dx=regs.x.dx;
	if (cx) *cx=regs.x.cx;
}

/* The following function determines which character is used for
 * commandline-options by command.com. This system call is undocumented
 * and valid for versions < 4.00 only.
 */
 
int switchar()
{
	union REGS regs;
	regs.x.ax=0x3700;
	int86(0x21, &regs, &regs);
	return regs.h.dl;
}

#endif
