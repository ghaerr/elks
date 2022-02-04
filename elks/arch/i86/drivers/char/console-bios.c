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

#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>
#include "console.h"
#include "console-bios.h"

/* Assumes ASCII values. */
#define isalpha(c) (((unsigned char)(((c) | 0x20) - 'a')) < 26)

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
    unsigned char attr;		/* current attribute */
    unsigned char XN;		/* delayed newline on column 80 */
    unsigned char color;	/* fg/bg attr */
#ifdef CONFIG_EMUL_VT52
    unsigned char tmp;		/* ESC Y ch save */
#endif
#ifdef CONFIG_EMUL_ANSI
    int savex, savey;		/* saved cursor position */
    unsigned char *parmptr;	/* ptr to params */
    unsigned char params[MAXPARMS];	/* ANSI params */
#endif
    int pageno;			/* video ram page # */
};

static struct wait_queue glock_wait;
static Console Con[MAX_CONSOLES], *Visible;
static Console *glock;		/* Which console owns the graphics hardware */
static int Width, MaxCol, Height, MaxRow;
static unsigned short int NumConsoles = MAX_CONSOLES;
static int kraw;
static int Current_VCminor = 0;

#ifdef CONFIG_EMUL_ANSI
#define TERM_TYPE " emulating ANSI "
#elif CONFIG_EMUL_VT52
#define TERM_TYPE " emulating vt52 "
#else
#define TERM_TYPE " dumb "
#endif

static void std_char(register Console *, char);

static void PositionCursor(register Console * C)
{
    int x, y, p;

    x = C->cx;
    y = C->cy;
    p = C->pageno;

    bios_setcursor (x, y, p);
}

static void PositionCursorGet (int * x, int * y)
{
	byte_t col, row;
	bios_getcursor (&col, &row);
	*x = col;
	*y = row;
}

static void VideoWrite(register Console * C, char c)
{
    int a, p;

    a = C->attr;
    p = C->pageno;
    bios_writecharattr (c, a, p);
}

static void scroll(register Console * C, int n, int x, int y, int xx, int yy)
{
    int a;

    a = C->attr;
    if (C != Visible) {
	bios_setpage(C->pageno);
    }

    bios_scroll (a, n, x, y, xx, yy);

    if (C != Visible) {
	bios_setpage(Visible->pageno);
    }
}

static void ClearRange(register Console * C, int x, int y, int xx, int yy)
{
    scroll(C, 26, x, y, xx, yy);
}

static void ScrollUp(register Console * C, int y)
{
    scroll(C, 1, 0, y, MaxCol, MaxRow);
}

#if defined (CONFIG_EMUL_VT52) || defined (CONFIG_EMUL_ANSI)
static void ScrollDown(register Console * C, int y)
{
    scroll(C, -1, 0, y, MaxCol, MaxRow);
}
#endif

/* shared console routines*/
#include "console.c"

/* This also tells the keyboard driver which tty to direct it's output to...
 * CAUTION: It *WILL* break if the console driver doesn't get tty0-X.
 */

void Console_set_vc(unsigned int N)
{
    if ((N >= NumConsoles) || (Visible == &Con[N]) || glock)
	return;
    Visible = &Con[N];

    bios_setpage(N);
    PositionCursor(Visible);
    Current_VCminor = N;
}

struct tty_ops bioscon_ops = {
    Console_open,
    Console_release,
    Console_write,
    NULL,
    Console_ioctl,
    Console_conout
};

void console_init(void)
{
    register Console *C;
    register int i;

    MaxCol = (Width = SETUP_VID_COLS) - 1;

    /* Trust this. Cga does not support peeking at 0x40:0x84. */
    MaxRow = (Height = SETUP_VID_LINES) - 1;

    if (peekb(0x49, 0x40) == 7)  /* BIOS data segment */
	NumConsoles = 1;

    C = Con;
    Visible = C;

    for (i = 0; i < NumConsoles; i++) {
	C->cx = C->cy = 0;
	if (!i) {
		// Get current cursor position
		// to write after boot messages
		PositionCursorGet (&C->cx, &C->cy);
	}
	C->fsm = std_char;
	C->pageno = i;
	C->attr = A_DEFAULT;
	C->color = A_DEFAULT;

#ifdef CONFIG_EMUL_ANSI

	C->savex = C->savey = 0;

#endif

	/* Do not erase early printk() */
	/* ClearRange(C, 0, C->cy, MaxCol, MaxRow); */

	C++;
    }

    kbd_init();

    printk("BIOS console %ux%u"TERM_TYPE"(%u virtual consoles)\n",
	   Width, Height, NumConsoles);
}
