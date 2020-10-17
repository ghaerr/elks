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

/* Assumes ASCII values. */
#define isalpha(c) (((unsigned char)(((c) | 0x20) - 'a')) < 26)

/* Function from bioscon-low */
/* TODO: move these functions to a BIOS library */

extern int poll_kbd ();
extern void SetDisplayPage (byte_t page);
extern void PosCursSetLow (byte_t x, byte_t y, byte_t page);
extern void PosCursGetLow (byte_t * x, byte_t * y);
extern void VideoWriteLow (byte_t c, byte_t attr, byte_t page);
extern void ScrollLow (byte_t attr, byte_t n, byte_t x, byte_t y, byte_t xx, byte_t yy);


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
static struct timer_list timer;

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

extern void AddQueue(unsigned char Key);
static void std_char(register Console *, char);
static void Console_set_vc(unsigned int);

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
 * Bios Keyboard Decoder
 */
static void kbd_timer(int __data)
{
    int dav, extra = 0;
#if 0
    static int clk[4] = {0x072D, 0x075C, 0x077C, 0x072F,};
    static int c = 0;

    pokew((79+0*80)*2, 0xB800, clk[(c++)&0x03]);
#endif
    if ((dav = poll_kbd())) {
	if (dav & 0xFF)
	    AddQueue(dav & 0x7F);
	else {
	    dav = (dav >> 8) & 0xFF;
	    if (dav >= 0x3B && dav <= 0x3D) {	/* temp console switch on F1-F3*/
		Console_set_vc(dav - 0x3B);
		dav = 0;
	    }
	    else if ((dav >= 0x68) && (dav < 0x6B)) {	/* Change VC */
		Console_set_vc((unsigned)(dav - 0x68));
		dav = 0;
	    }
	    else if (dav >= 0x3B && dav < 0x45)	/* Function keys */
		dav = dav - 0x3B + 'a';
	    else {
		switch(dav) {
		case 0x48: dav = 'A'; break;	/* up*/
		case 0x50: dav = 'B'; break;	/* down*/
		case 0x4d: dav = 'C'; break;	/* right*/
		case 0x4b: dav = 'D'; break;	/* left*/
		case 0x47: dav = 'H'; break;	/* home*/
		case 0x4f: dav = 'F'; break;	/* end*/
		case 0x49: dav = '5'; extra = '~'; break; /* pgup*/
		case 0x51: dav = '6'; extra = '~'; break; /* pgdn*/
		default:   dav = 0;
		}
	    }
	    if (dav) {
		AddQueue(ESC);
#ifdef CONFIG_EMUL_ANSI
		AddQueue('[');
#endif
		AddQueue(dav);
#ifdef CONFIG_EMUL_ANSI
		if (extra)
		    AddQueue(extra);
#endif
	    }
	}
    }
    restart_timer();
}

static void PositionCursor(register Console * C)
{
    int x, y, p;

    x = C->cx;
    y = C->cy;
    p = C->pageno;

    PosCursSetLow (x, y, p);
}

static void PositionCursorGet (int * x, int * y)
{
	byte_t col, row;
	PosCursGetLow (&col, &row);
	*x = col;
	*y = row;
}

static void VideoWrite(register Console * C, char c)
{
    int a, p;

    a = C->attr;
    p = C->pageno;
    VideoWriteLow (c, a, p);
}

static void scroll(register Console * C, int n, int x, int y, int xx, int yy)
{
    int a;

    a = C->attr;
    if (C != Visible) {
	SetDisplayPage(C->pageno);
    }

    ScrollLow (a, n, x, y, xx, yy);

    if (C != Visible) {
	SetDisplayPage(Visible->pageno);
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

static void Console_set_vc(unsigned int N)
{
    if ((N >= NumConsoles) || (Visible == &Con[N]) || glock)
	return;
    Visible = &Con[N];

    SetDisplayPage(N);
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

    MaxCol = (Width = setupb(7)) - 1;

    /* Trust this. Cga does not support peeking at 0x40:0x84. */
    MaxRow = (Height = setupb(14)) - 1;

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
    printk("BIOS console %ux%u"TERM_TYPE"(%u virtual consoles)\n",
	   Width, Height, NumConsoles);
}

#endif
