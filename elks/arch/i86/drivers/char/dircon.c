/*
 * Direct video memory display driver
 * Saku Airila 1996
 * Color originally written by Mbit and Davey
 * Re-wrote for new ntty iface
 * Al Riddoch  1999
 *
 * Rewritten by Greg Haerr <greg@censoft.com> July 1999
 * added reverse video, cleaned up code, reduced size
 * added enough ansi escape sequences for visual editing
 */
#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>
#include <linuxmt/major.h>
#include <linuxmt/sched.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>

#ifdef CONFIG_CONSOLE_DIRECT

#include "console.h"

#define islower(c) ((c) >= 'a' && (c) <= 'z')
#define isupper(c) ((c) >= 'A' && (c) <= 'Z')

#if 0
/* public interface of dircon.c: */

void con_charout(char Ch);
void Console_set_vc(int N);
int Console_write(struct tty *tty);
void Console_release(struct tty *tty);
int Console_open(struct tty *tty);
struct tty_ops dircon_ops;
void init_console(void);
#endif

#define MAX_ATTR 	5
#define A_DEFAULT 	0x07
#define A_BOLD 		0x08
#define A_BLINK 	0x80
#define A_REVERSE	0x70
#define A_BLANK		0x00

/* character definitions*/
#define BS		'\b'
#define NL		'\n'
#define CR		'\r'
#define TAB		'\t'
#define ESC		27
#define BEL		0x07

/* console states*/
#define ST_NORMAL	0	/* no ESC seen */
#define ST_ESCSEEN	1	/* ESC seen */
#define ST_ANSI		2	/* ESC [ seen */
#define ST_ESCP		3	/* ESC P seen */
#define ST_ESCQ		4	/* ESC Q seen */
#define	ST_ESCY		5	/* ESC Y seen */
#define ST_ESCY2	6	/* ESC Y ch seen */

#define MAXPARMS	10
typedef struct {
    int cx, cy;			/* cursor position */
    int state;			/* escape state */
    unsigned int vseg;		/* video segment for page */
    int pageno;			/* video ram page # */
    unsigned char attr;		/* current attribute */
#ifdef CONFIG_DCON_VT52
    unsigned char tmp;		/* ESC Y ch save */
#ifdef CONFIG_DCON_ANSI
    int savex, savey;		/* saved cursor position */
    unsigned char *parmptr;	/* ptr to params */
    unsigned char params[MAXPARMS];	/* ANSI params */
#endif
#endif
} Console;

static int Width, Height, MaxRow, MaxCol;
static int CCBase;
static int NumConsoles = MAX_CONSOLES;
static unsigned VideoSeg, PageSize;
static Console Con[MAX_CONSOLES];
static Console *Visible;
static Console *glock;		/* Which console owns the graphics hardware */
static struct wait_queue glock_wait;

/* from keyboard.c */
extern int Current_VCminor;
extern int kraw;

#ifdef CONFIG_DCON_VT52
static unsigned AttrArry[MAX_ATTR] = {
    A_DEFAULT,
    A_BOLD,
    A_BLINK,
    A_REVERSE,
    A_BLANK
};
#endif

static void ScrollUp();
static void PositionCursor();
static void WriteChar();
static void ClearRange();
#ifdef CONFIG_DCON_VT52
static void ScrollDown();
static void Vt52Cmd();
static void Vt52CmdEx();
static void Console_gotoxy();
#ifdef CONFIG_DCON_ANSI
static void AnsiCmd();
#endif
#endif

extern int peekb();
extern int peekw();
extern void pokeb();
extern void pokew();
extern void outb();

extern int AddQueue();		/* From xt_key.c */
extern int GetQueue();

void con_charout(char Ch)
{
    WriteChar(Visible, Ch);
    PositionCursor(Visible);
}

void WriteChar(register Console * cons, char c)
{
    unsigned int offset;

    /* check for graphics lock */
    while (glock) {
	if (glock == cons)
	    return;
	sleep_on(&glock_wait);
    }

#ifdef CONFIG_DCON_ANSI
    /* ANSI param gathering and processing */
    if (cons->state == ST_ANSI) {
	if (cons->parmptr < &cons->params[MAXPARMS])
	    *cons->parmptr++ = c;
	if (isupper(c) || islower(c)) {
	    *cons->parmptr = 0;
	    AnsiCmd(cons, c);
	}
	return;
    }
#endif

#ifdef CONFIG_DCON_VT52
    /* VT52 command processing */
    if (cons->state >= ST_ESCSEEN) {
#ifdef CONFIG_DCON_ANSI
	if (c == '[') {
	    cons->state = ST_ANSI;
	    cons->parmptr = cons->params;
	    return;
	}
#endif
	Vt52Cmd(cons, c);
	return;
    }
#endif

    /* normal character processing */
    switch (c) {
    case BEL:
	bell();
	return;
#ifdef CONFIG_DCON_VT52
    case ESC:
	cons->state = ST_ESCSEEN;
	return;
#endif
    case BS:
	if (cons->cx > 0) {
	    --cons->cx;
	    WriteChar(cons, ' ');
	    --cons->cx;
	    PositionCursor(cons);
	}
	return;
    case NL:
	++cons->cy;
	/* fall thru for now: fixme when ONLCR complete */
    case CR:
	cons->cx = 0;
	break;
    default:
	offset = (cons->cx + cons->cy * Width) << 1;
	pokeb(cons->vseg, offset++, c);
	pokeb(cons->vseg, offset, cons->attr);
	++cons->cx;
    }

    /* autowrap and/or scroll */
    if (cons->cx > MaxCol) {
	cons->cx = 0;
	++cons->cy;
    }
    if (cons->cy >= Height) {
	ScrollUp(cons, 1, Height);
	--cons->cy;
    }
}

void ScrollUp(register Console * C, int st, int en)
{
    unsigned rdofs, wrofs;
    int cnt;

    if (st < 1 || st >= en)
	return;
    rdofs = (Width << 1) * st;
    wrofs = rdofs - (Width << 1);
    cnt = Width * (en - st);
    far_memmove(C->vseg, rdofs, C->vseg, wrofs, cnt << 1);
    en--;
    ClearRange(C, 0, en, Width, en);
}

#ifdef CONFIG_DCON_VT52

void ScrollDown(register Console * C, int st, int en)
{
    unsigned rdofs, wrofs;
    int cnt;

    if (st > en || en > Height)
	return;
    rdofs = (Width << 1) * st;
    wrofs = rdofs + (Width << 1);
    cnt = Width * (en - st);
    far_memmove(C->vseg, rdofs, C->vseg, wrofs, cnt << 1);
    ClearRange(C, 0, st, Width, st);
}

#endif

void PositionCursor(register Console * C)
{
    int Pos;
    if (C != Visible)
	return;
    Pos = C->cx + Width * C->cy + (C->pageno * PageSize >> 1);
    outb(14, CCBase);
    outb((unsigned char) ((Pos >> 8) & 0xFF), CCBase + 1);
    outb(15, CCBase);
    outb((unsigned char) (Pos & 0xFF), CCBase + 1);
}

void ClearRange(register Console * C, int x, int y, int xx, int yy)
{
    unsigned st, en, ClrW, ofs;
    st = (x + y * Width) << 1;
    en = (xx + yy * Width) << 1;
    ClrW = A_DEFAULT << 8 + ' ';
    for (ofs = st; ofs < en; ofs += 2)
	pokew(C->vseg, ofs, ClrW);
}

#ifdef CONFIG_DCON_VT52

void Vt52Cmd(register Console * cons, char c)
{
    /* process multi character ESC sequences */
    if (cons->state > ST_ESCSEEN) {
	Vt52CmdEx(cons, c);
	return;
    }

    /* process single ESC char sequences */
    switch (c) {
    case 'H':			/* home */
	cons->cx = cons->cy = 0;
	break;
    case 'A':			/* up */
	if (cons->cy)
	    --cons->cy;
	break;
    case 'B':			/* down */
	if (cons->cy < MaxRow)
	    ++cons->cy;
	break;
    case 'C':			/* right */
	if (cons->cx < MaxCol)
	    ++cons->cx;
	break;
    case 'D':			/* left */
	if (cons->cx)
	    --cons->cx;
	break;
    case 'K':			/* clear to eol */
	ClearRange(cons, cons->cx, cons->cy, Width, cons->cy);
	break;
    case 'J':			/* clear to eoscreen */
	ClearRange(cons, 0, 0, Width, Height);
	break;
    case 'I':			/* linefeed reverse */
	ScrollDown(cons, 0, MaxRow);
	break;
    case 'P':			/* NOT VT52. insert/delete line */
	cons->state = ST_ESCP;
	return;
    case 'Q':			/* NOT VT52. My own additions. Attributes... */
	cons->state = ST_ESCQ;
	return;
    case 'Y':			/* cursor move */
	cons->state = ST_ESCY;
	return;

#if 0

    case 'Z':			/* identify */
	/* Problem here */
	AddQueue(27);
	AddQueue('Z');
	break;

#endif

    }
    cons->state = ST_NORMAL;
}

void Vt52CmdEx(register Console * cons, char c)
{
    switch (cons->state) {
    case ST_ESCY:
	cons->tmp = c - ' ';
	cons->state = ST_ESCY2;
	return;
    case ST_ESCY2:
	Console_gotoxy(cons, c - ' ', cons->tmp);
	break;
    case ST_ESCP:
	switch (c) {
	case 'f':		/* insert line at crsr */
	    ScrollDown(cons, cons->cy, MaxRow);
	    break;
	case 'd':		/* delete line at crsr */
	    ScrollUp(cons, cons->cy + 1, Height);
	    break;
	}
	break;
    case ST_ESCQ:
	c -= ' ';
	if (c > 0 && c < MAX_ATTR)
	    cons->attr |= AttrArry[c - 1];
	if (c == 0)
	    cons->attr = A_DEFAULT;
	break;
    }
    cons->state = ST_NORMAL;
}

#endif

#ifdef CONFIG_DCON_ANSI

static int parm1(char *buf)
{
    int n = atoi(buf);

    if (n == 0)
	return 1;
    return n;
}

static int parm2(register char *buf)
{
    if (*buf != ';')
	while (*buf && *buf != ';')
	    ++buf;
    if (*buf)
	++buf;
    return parm1(buf);
}

void AnsiCmd(register Console * cons, char c)
{
    register unsigned char *p;
    int n;

    switch (c) {
    case 's':			/* Save the current location */
	cons->savex = cons->cx;
	cons->savey = cons->cy;
	break;
    case 'u':			/* Restore saved location */
	cons->cx = cons->savex;
	cons->cy = cons->savey;
	break;
    case 'A':			/* Move up n lines */
	n = parm1(cons->params);
	cons->cy -= n;
	if (cons->cy < 0)
	    cons->cy = 0;
	break;
    case 'B':			/* Move down n lines */
	n = parm1(cons->params);
	cons->cy += n;
	if (cons->cy > MaxRow)
	    cons->cy = MaxRow;
	break;
    case 'C':			/* Move right n characters */
	n = parm1(cons->params);
	cons->cx += n;
	if (cons->cx > MaxCol)
	    cons->cx = MaxCol;
	break;
    case 'D':			/* Move left n characters */
	n = parm1(cons->params);
	cons->cx -= n;
	if (cons->cx < 0)
	    cons->cx = 0;
	break;
    case 'H':			/* cursor move */
	Console_gotoxy(cons, parm2(cons->params) - 1, parm1(cons->params) - 1);
	break;
    case 'J':			/* erase */
	if (parm1(cons->params) == 2)
	    ClearRange(cons, 0, 0, Width, Height);
	break;
    case 'K':			/* Clear to EOL */
	ClearRange(cons, cons->cx, cons->cy, Width, cons->cy);
	break;
    case 'm':			/* ansi color */
	cons->state = ST_NORMAL;
	p = cons->params;
	for (;;) {
	    switch (*p++) {
	    case 'm':
		if (p == &cons->params[1])
		    cons->attr = A_DEFAULT;
		return;
	    case ';':
		continue;
	    case '0':
		cons->attr = A_DEFAULT;
		continue;
	    case '1':
		cons->attr |= A_BOLD;
		continue;
	    case '5':
		cons->attr |= A_BLINK;
		continue;
	    case '7':
		cons->attr = A_REVERSE;
		continue;
	    case '8':
		cons->attr = A_BLANK;
		continue;
	    case '3':
		if (*p >= '0' && *p <= '7') {
		    cons->attr &= 0xf8;
		    cons->attr |= *p++ & 0x07;
		    continue;
		}
		cons->attr = A_DEFAULT;
		return;
	    case '4':
		if (*p >= '0' && *p <= '7') {
		    cons->attr &= 0x8f;
		    cons->attr |= (*p++ << 4) & 0x70;
		    continue;
		}
		/* fall thru */
	    default:
		cons->attr = A_DEFAULT;
		return;
	    }
	}
	break;
    }
    cons->state = ST_NORMAL;
}

#endif

/* This also tells the keyboard driver which tty to direct it's output to...
 * CAUTION: It *WILL* break if the console driver doesn't get tty0-X. 
 */

void Console_set_vc(int N)
{
    unsigned int offset;

    if (N < 0 || N >= NumConsoles)
	return;
    if (Visible == &Con[N])
	return;
    if (glock)
	return;
    Visible = &Con[N];
#if 0
    far_memmove(Visible->vseg, 0, VideoSeg, 0, (Width * Height) << 1);
#endif
    offset = N * PageSize >> 1;
    outw((offset & 0xff00) | 0x0c, CCBase);
    outw(((offset & 0xff) << 8) | 0x0d, CCBase);
    PositionCursor(Visible);
    Current_VCminor = N;
}

#ifdef CONFIG_DCON_VT52

void Console_gotoxy(register Console * cons, int x, int y)
{
    if (x >= MaxCol)
	x = MaxCol;
    if (y >= MaxRow)
	y = MaxRow;
    if (x < 0)
	x = 0;
    if (y < 0)
	y = 0;
    cons->cx = x;
    cons->cy = y;
    PositionCursor(cons);
}

#endif

int Console_ioctl(struct tty *tty, int cmd, char *arg)
{
    switch (cmd) {
    case DCGET_GRAPH:
	if (!glock) {
	    glock = &Con[tty->minor];
	    return 0;
	}
	return -EBUSY;
    case DCREL_GRAPH:
	if (glock == &Con[tty->minor]) {
	    glock = NULL;
	    wake_up(&glock_wait);
	    return 0;
	}
	return -EINVAL;
    case DCSET_KRAW:
	if (glock == &Con[tty->minor]) {
	    kraw = 1;
	    return 0;
	}
	return -EINVAL;
    case DCREL_KRAW:
	if ((glock == &Con[tty->minor]) && (kraw == 1)) {
	    kraw = 0;
	    return 1;
	}
	return -EINVAL;
    }

    return -EINVAL;
}

int Console_write(register struct tty *tty)
{
    int cnt = 0, chi;
    unsigned char ch;
    Console *C = &Con[tty->minor];
    while (tty->outq.len != 0) {
	chq_getch(&tty->outq, &ch, 0);
#if 0
	if (ch == '\n')
	    WriteChar(C, '\r');
#endif
	WriteChar(C, ch);
	cnt++;
    };
    PositionCursor(C);
    return cnt;
}

void Console_release(struct tty *tty)
{
/* Do nothing */
}

int Console_open(struct tty *tty)
{
    if (tty->minor >= NumConsoles)
	return -ENODEV;
    return 0;
}

struct tty_ops dircon_ops = {
    Console_open,
    Console_release,
    Console_write,
    NULL,
    Console_ioctl,
};

void init_console(void)
{
    unsigned int i;
    register Console *cons;

    MaxCol = (Width = peekb(0x40, 0x4a)) - 1;

    /* Trust this. Cga does not support peeking at 0x40:0x84. */
    MaxRow = (Height = 25) - 1;
    CCBase = peekw(0x40, 0x63);
    PageSize = peekw(0x40, 0x4C);
    if (peekb(0x40, 0x49) == 7) {
	VideoSeg = 0xB000;
	NumConsoles = 1;
    } else
	VideoSeg = 0xb800;

    for (i = 0; i < NumConsoles; i++) {
	cons = &Con[i];
	cons->cx = cons->cy = 0;
	cons->state = ST_NORMAL;
	cons->vseg = VideoSeg + (PageSize >> 4) * i;
	cons->pageno = i;
	cons->attr = A_DEFAULT;

#ifdef CONFIG_DCON_ANSI

	cons->savex = cons->savey = 0;

#endif

	if (i != 0)
	    ClearRange(cons, 0, 0, Width, Height);
    }

    cons = &Con[0];
    cons->cx = peekb(0x40, 0x50);
    cons->cy = peekb(0x40, 0x51);
    Visible = cons;

#ifdef CONFIG_DCON_VT52

    printk("Console: Direct %ux%u emulating vt52 (%u virtual consoles)\n",
	   Width, Height, NumConsoles);

#else

    printk("Console: Direct %ux%u dumb (%u virtual consoles)\n",
	   Width, Height, NumConsoles);

#endif
}

#endif
