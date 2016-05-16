/*
 * Bios display driver
 * Saku Airila 1996
 * Color originally written by Mbit and Davey
 * Re-wrote for new ntty iface
 * Al Riddoch  1999
 *
 * Rewritten by Greg Haerr <greg@censoft.com> July 1999
 * added reverse video, cleaned up code, reduced size
 * added enough ansi escape sequences for visual editing
 */
/*
 * FIXME: Don't use inline assembly to permit the use of ia16-GCC
 */
/*
 * FIXME: Keyboard driver extremely barebones. Only recognizes ASCII codes.
 */
#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/mm.h>
#include <linuxmt/timer.h>
#include <linuxmt/sched.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>

#include <arch/io.h>
#include <arch/segment.h>

#ifdef CONFIG_CONSOLE_BIOS

#include "console.h"

/* Assumes ASCII values. */
#define isalpha(c) (((unsigned char)(((c) | 0x20) - 'a')) < 26)

#if 0
/* public interface of bioscon.c: */

void con_charout(char Ch);
int Console_write(struct tty *tty);
void Console_release(struct tty *tty);
int Console_open(struct tty *tty);
struct tty_ops dircon_ops;
void init_console(void);
#endif

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
#define ESC		'\x1B'
#define BEL		'\x07'

#define MAXPARMS	10

struct console;
typedef struct console Console;

struct console {
    int cx, cy;			/* cursor position */
    void (*fsm)(register Console *, char);
    int pageno;			/* video ram page # */
    unsigned char attr;		/* current attribute */
#ifdef CONFIG_EMUL_VT52
    unsigned char tmp;		/* ESC Y ch save */
#endif
#ifdef CONFIG_EMUL_ANSI
    int savex, savey;		/* saved cursor position */
    unsigned char *parmptr;	/* ptr to params */
    unsigned char params[MAXPARMS];	/* ANSI params */
#endif
};

static struct wait_queue glock_wait;
static Console Con[MAX_CONSOLES], *Visible;
static Console *glock;		/* Which console owns the graphics hardware */
static int Width, MaxCol, Height, MaxRow;
static unsigned short int NumConsoles = MAX_CONSOLES;
static struct timer_list timer;

extern struct tty ttys[];

/* for keyboard.c */
int Current_VCminor = 0;
int kraw = 0;

#ifdef CONFIG_EMUL_ANSI
#define TERM_TYPE " emulating ANSI "
#elif CONFIG_EMUL_VT52
#define TERM_TYPE " emulating vt52 "
#else
#define TERM_TYPE " dumb "
#endif

static void std_char(register Console *, char);

static void AddQueue(unsigned char Key)
{
    register struct tty *ttyp = &ttys[Current_VCminor];

    if(Key == '\r')
	Key = '\n';
    if (!tty_intcheck(ttyp, Key) && (ttyp->inq.size != 0))
	chq_addch(&ttyp->inq, Key, 0);
}

static void kbd_timer(int __data);
/*
 * Restart timer
 */
static void restart_timer(void)
{
    init_timer(&timer);
    timer.tl_expires = jiffies + 8;
    timer.tl_function = kbd_timer;
    add_timer(&timer);
}

/*
 * Start Bios Keyboard.
 */
void xtk_init(void)
{
    enable_irq(1);		/* enable BIOS Keyboard interrupts */
    restart_timer();
}

/*
 * Bios Keyboard Poll
 */
static void kbd_timer(int __data)
{
    int		dav;
#if 0
    static int clk[4] = {0x072D, 0x075C, 0x077C, 0x072F,};
    static int c = 0;

    pokew(0xB800, (79+0*80)*2, clk[(c++)&0x03]);
#endif
#asm
    jmp		nhp
savekey:
    xor		ah,ah
    int		0x16
    push	ax
    call	_AddQueue
    add		sp,#2
nhp:
    mov		ah, #0x01
    int		0x16
    xor		ah,ah
    cmp		al,#6
    jge		savekey
#endasm
    restart_timer();
}

/*
 *      Busy wait for a keypress in kernel state for bootup/debug.
 */

int wait_for_keypress(void)
{
    set_irq();
    return chq_getch(&ttys[0].inq, 0, 1);
}

static void SetDisplayPage(unsigned int n)
{
    int		x;
#asm
    mov		ah, #0x05
    mov		al, [bp + .SetDisplayPage.n]
    int		#0x10
#endasm
}

static void PositionCursor(register Console * C)
{
    int x, y, p;

    x = C->cx;
    y = C->cy;
    p = C->pageno;
#asm
    mov		ah, #0x02
    mov		bh, [bp + .PositionCursor.p]
    mov		dh, [bp + .PositionCursor.y]
    mov		dl, [bp + .PositionCursor.x]
    int		#0x10
#endasm
}

static void VideoWrite(register Console * C, char c)
{
    int a, p;

    a = C->attr;
    p = C->pageno;
#asm
    mov		ah, #0x09
    mov		al, [bp + .VideoWrite.c]
    mov		bh, [bp + .VideoWrite.p]
    mov		bl, [bp + .VideoWrite.a]
    mov		cx, #1
    int		#0x10
#endasm
}

static void genscro(register Console * C, int n, int x, int y, int xx, int yy)
{
    int a, p;

    a = C->attr;
    p = C->pageno;
    if(C != Visible) {
#asm
    mov		ah, #0x05
    mov		al, [bp + .genscro.p]
    int		#0x10
#endasm
    }
#asm
    mov		ah, #0x06
    mov		al, [bp + .genscro.n]
    cmp		al, #0
    jge		scrup
    inc		ah
    neg		al
scrup:
    mov		bh, [bp + .genscro.a]
    mov		ch, [bp + .genscro.y]
    mov		cl, [bp + .genscro.x]
    mov		dh, [bp + .genscro.yy]
    mov		dl, [bp + .genscro.xx]
    int		#0x10
#endasm
    if(C != Visible) {
	p = Visible->pageno;
#asm
    mov		ah, #0x05
    mov		al, [bp + .genscro.p]
    int		#0x10
#endasm
    }
}

static void ClearRange(register Console * C, int x, int y, int xx, int yy)
{
    genscro(C, 26, x, y, xx, yy);
}

static void ScrollUp(register Console * C, int y)
{
    genscro(C, 1, 0, y, MaxCol, MaxRow);
}

#if defined (CONFIG_EMUL_VT52) || defined (CONFIG_EMUL_ANSI)
static void ScrollDown(register Console * C, int y)
{
    genscro(C, -1, 0, y, MaxCol, MaxRow);
}
#endif

#if defined (CONFIG_EMUL_VT52) || defined (CONFIG_EMUL_ANSI)
static void Console_gotoxy(register Console * C, int x, int y)
{
    register char *xp = (char *)x;

    C->cx = ((((int) xp) >= MaxCol) ? MaxCol : ((((int)xp) < 0) ? 0 : (int)xp));
    xp = (char *)y;
    C->cy = ((((int) xp) >= MaxRow) ? MaxRow : ((((int)xp) < 0) ? 0 : (int)xp));
}
#endif

#ifdef CONFIG_EMUL_ANSI
static int parm1(register unsigned char *buf)
{
    register char *np;

    np = (char *) atoi((char *) buf);
    return (np) ? ((int) np) : 1;

}

static int parm2(register unsigned char *buf)
{
    while (*buf != ';' && *buf)
	buf++;
    if (*buf)
	buf++;
    return parm1(buf);
}

static void AnsiCmd(register Console * C, char c)
{
    register unsigned char *p;

    /* ANSI param gathering and processing */
    if (C->parmptr < &C->params[MAXPARMS - 1])
	*C->parmptr++ = (unsigned char) c;
    if (!isalpha(c)) {
	return;
    }
    *(C->parmptr) = 0;

    switch (c) {
    case 's':			/* Save the current location */
	C->savex = C->cx;
	C->savey = C->cy;
	break;
    case 'u':			/* Restore saved location */
	C->cx = C->savex;
	C->cy = C->savey;
	break;
    case 'A':			/* Move up n lines */
	C->cy -= parm1(C->params);
	if (C->cy < 0)
	    C->cy = 0;
	break;
    case 'B':			/* Move down n lines */
	C->cy += parm1(C->params);
	if (C->cy > MaxRow)
	    C->cy = MaxRow;
	break;
    case 'C':			/* Move right n characters */
	C->cx += parm1(C->params);
	if (C->cx > MaxCol)
	    C->cx = MaxCol;
	break;
    case 'D':			/* Move left n characters */
	C->cx -= parm1(C->params);
	if (C->cx < 0)
	    C->cx = 0;
	break;
    case 'H':			/* cursor move */
	Console_gotoxy(C, parm2(C->params) - 1, parm1(C->params) - 1);
	break;
    case 'J':			/* erase */
	if (parm1(C->params) == 2)
	    ClearRange(C, 0, 0, MaxCol, MaxRow);
	break;
    case 'K':			/* Clear to EOL */
	ClearRange(C, C->cx, C->cy, MaxCol, C->cy);
	break;
    case 'L':			/* insert line */
	ScrollDown(C, C->cy);
	break;
    case 'M':			/* remove line */
	ScrollUp(C, C->cy);
	break;
    case 'm':			/* ansi color */
	p = C->params;
	for (;;) {
	    switch (*p++) {
	    case ';':
		continue;
	    case '0':
		C->attr = A_DEFAULT;
		continue;
	    case '1':
		C->attr |= A_BOLD;
		continue;
	    case '5':
		C->attr |= A_BLINK;
		continue;
	    case '7':
		C->attr = A_REVERSE;
		continue;
	    case '8':
		C->attr = A_BLANK;
		continue;

	    case '4':				/* Set background color */
		if (*p >= '0' && *p <= '7') {
		    C->attr &= 0x8f;
		    C->attr |= (*p++ << 4) & 0x70;
		    continue;
		}
		/* fall thru */
	    case 'm':
		if (p != &C->params[1])
		    break;
	    case '3':				/* Set foreground color */
		if (*p >= '0' && *p <= '7') {
		    C->attr &= 0xf8;
		    C->attr |= *p++ & 0x07;
		    continue;
		}
	    default:
		C->attr = A_DEFAULT;
	    }
	    break;
	}
    }
    C->fsm = std_char;
}

/* Escape character processing */
static void esc_char(register Console * C, char c)
{
    /* Parse CSI sequence */
    C->parmptr = C->params;
    C->fsm = (c == '[' ? AnsiCmd : std_char);
}
#endif

#ifdef CONFIG_EMUL_VT52
static void esc_Y2(register Console * C, char c)
{
    Console_gotoxy(C, c - ' ', C->tmp);
    C->fsm = std_char;
}

static void esc_YS(register Console * C, char c)
{
    switch(C->tmp) {
    case 'Y':
	C->tmp = (unsigned char) (c - ' ');
	C->fsm = esc_Y2;
	break;
    case '/':
	C->tmp = 'Z';		/* Discard next char */
	break;
    case 'Z':
	C->fsm = std_char;
	break;
    }
}

/* Escape character processing */
static void esc_char(register Console * C, char c)
{
    /* process single ESC char sequences */
    C->fsm = std_char;
    switch (c) {
    case 'I':			/* linefeed reverse */
	if (!C->cy) {
	    ScrollDown(C, 0);
	    break;
	}
    case 'A':			/* up */
	if (C->cy)
	    --C->cy;
	break;
    case 'B':			/* down */
	if (C->cy < MaxRow)
	    ++C->cy;
	break;
    case 'C':			/* right */
	if (C->cx < MaxCol)
	    ++C->cx;
	break;
    case 'D':			/* left */
	if (C->cx)
	    --C->cx;
	break;
    case 'H':			/* home */
	C->cx = C->cy = 0;
	break;
    case 'J':			/* clear to eoscreen */
	ClearRange(C, 0, C->cy+1, MaxCol, MaxRow);
    case 'K':			/* clear to eol */
	ClearRange(C, C->cx, C->cy, MaxCol, C->cy);
	break;
    case 'L':			/* insert line */
	ScrollDown(C, C->cy);
	break;
    case 'M':			/* remove line */
	ScrollUp(C, C->cy);
	break;
    case '/':			/* Remove echo for identify response */
    case 'Y':			/* cursor move */
	C->tmp = c;
	C->fsm = esc_YS;
	break;
    case 'Z':			/* identify as VT52 */
	AddQueue(ESC);
	AddQueue('/');
	AddQueue('K');
    }
}
#endif

/* Normal character processing */
static void std_char(register Console * C, char c)
{
    switch(c) {
    case BEL:
	bell();
	break;
    case '\b':
	if (C->cx > 0) {
	    --C->cx;
	    VideoWrite(C, ' ');
	}
	break;
    case '\t':
	C->cx = (C->cx | 0x07) + 1;
	goto linewrap;
    case '\n':
	++C->cy;
	break;
    case '\r':
	C->cx = 0;
	break;

#if defined (CONFIG_EMUL_VT52) || defined (CONFIG_EMUL_ANSI)
    case ESC:
	C->fsm = esc_char;
	break;
#endif

    default:
	VideoWrite(C, c);
	C->cx++;
      linewrap:
	if(C->cx > MaxCol) {

#ifdef CONFIG_EMUL_VT52
	    C->cx = MaxCol;
#else
	    C->cx = 0;
	    C->cy++;
#endif

	}
    }
    if (C->cy > MaxRow) {
	ScrollUp(C, 0);
	C->cy = MaxRow;
    }
}

static void WriteChar(register Console * C, char c)
{
    /* check for graphics lock */
    while (glock) {
	if (glock == C)
	    return;
	sleep_on(&glock_wait);
    }
    C->fsm(C, c);
}

void con_charout(char Ch)
{
    if (Ch == '\n')
	WriteChar(Visible, '\r');
    WriteChar(Visible, Ch);
    PositionCursor(Visible);
}

/* This also tells the keyboard driver which tty to direct it's output to...
 * CAUTION: It *WILL* break if the console driver doesn't get tty0-X.
 */

static void Console_set_vc(unsigned int N)
{
    if ((N >= NumConsoles)
	|| (Visible == &Con[N])
	|| (glock))
	return;
    Visible = &Con[N];

    SetDisplayPage(N);
    PositionCursor(Visible);
    Current_VCminor = (int) N;
}

static int Console_ioctl(register struct tty *tty, int cmd, char *arg)
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
	break;
    case DCSET_KRAW:
	if (glock == &Con[tty->minor]) {
	    kraw = 1;
	    return 0;
	}
	break;
    case DCREL_KRAW:
	if ((glock == &Con[tty->minor]) && (kraw == 1)) {
	    kraw = 0;
	    return 1;
	}
/*  	break; */
    }

    return -EINVAL;
}

static int Console_write(register struct tty *tty)
{
    register Console *C = &Con[tty->minor];
    int cnt = 0;
    unsigned char ch;

    while (tty->outq.len != 0) {
	chq_getch(&tty->outq, &ch, 0);
	WriteChar(C, (char) ch);
	cnt++;
    }
    if(C == Visible)
	PositionCursor(C);
    return cnt;
}

static void Console_release(struct tty *tty)
{
/* Do nothing */
}

int Console_open(register struct tty *tty)
{
    return (tty->minor >= NumConsoles) ? -ENODEV : 0;
}

struct tty_ops bioscon_ops = {
    Console_open,
    Console_release,
    Console_write,
    NULL,
    Console_ioctl,
};

void init_console(void)
{
    register Console *C;
    register char *pi;

    MaxCol = (Width = setupb(7)) - 1;

    /* Trust this. Cga does not support peeking at 0x40:0x84. */
    MaxRow = (Height = setupb(14)) - 1;

    if (peekb(0x40, 0x49) == 7)
	NumConsoles = 1;

    C = Con;
    Visible = C;

    for (pi = 0; ((unsigned int)pi) < NumConsoles; pi++) {
	C->cx = C->cy = 0;
	if(!pi) {
	    C->cx = peekb(0x40, 0x50);
	    C->cy = peekb(0x40, 0x51);
	}
	C->fsm = std_char;
	C->pageno = (int) pi;
	C->attr = A_DEFAULT;

#ifdef CONFIG_EMUL_ANSI

	C->savex = C->savey = 0;

#endif

	ClearRange(C, 0, C->cy, MaxCol, MaxRow);
	C++;
    }

    printk("Console: BIOS %ux%u"TERM_TYPE"(%u virtual consoles)\n",
	   Width, Height, NumConsoles);
}

#endif
