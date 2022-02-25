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
#include <linuxmt/mm.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>
#include <arch/io.h>
#include "console.h"

/* Assumes ASCII values. */
#define isalpha(c) (((unsigned char)(((c) | 0x20) - 'a')) < 26)

#define A_DEFAULT 	0x07
#define A_BOLD 		0x08
#define A_UNDERLINE 	0x07	/* only works on MDA, normal video on EGA */
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
    unsigned int vseg;		/* video segment for page */
    int basepage;		/* start of video ram */
};

static struct wait_queue glock_wait;
static Console Con[MAX_CONSOLES], *Visible;
static Console *glock;		/* Which console owns the graphics hardware */
static void *CCBase;
static int Width, MaxCol, Height, MaxRow;
static unsigned short int NumConsoles = MAX_CONSOLES;

int Current_VCminor = 0;
int kraw = 0;
unsigned VideoSeg = 0xB800;

#ifdef CONFIG_EMUL_ANSI
#define TERM_TYPE " emulating ANSI "
#elif CONFIG_EMUL_VT52
#define TERM_TYPE " emulating vt52 "
#else
#define TERM_TYPE " dumb "
#endif

static void std_char(register Console *, char);

static void SetDisplayPage(register Console * C)
{
    register char *CCBasep;

    CCBasep = (char *) CCBase;
    outw((unsigned short int) ((C->basepage & 0xff00) | 0x0c), CCBasep);
    outw((unsigned short int) ((C->basepage << 8) | 0x0d), CCBasep);
}

static void PositionCursor(register Console * C)
{
    register char *CCBasep = (char *) CCBase;
    int Pos;

    Pos = C->cx + Width * C->cy + C->basepage;
    outb(14, CCBasep);
    outb((unsigned char) ((Pos >> 8) & 0xFF), CCBasep + 1);
    outb(15, CCBasep);
    outb((unsigned char) (Pos & 0xFF), CCBasep + 1);
}

static void VideoWrite(register Console * C, char c)
{
    pokew((word_t)((C->cx + C->cy * Width) << 1),
    		(seg_t) C->vseg,
	  ((word_t)C->attr << 8) | ((word_t)c));
}

static void ClearRange(register Console * C, int x, int y, int xx, int yy)
{
    register __u16 *vp;

    xx = xx - x + 1;
    vp = (__u16 *)((__u16)(x + y * Width) << 1);
    do {
	for (x = 0; x < xx; x++)
	    pokew((word_t) (vp++), (seg_t) C->vseg, (((word_t) C->attr << 8) | ' '));
	vp += (Width - xx);
    } while (++y <= yy);
}

static void ScrollUp(register Console * C, int y)
{
    register __u16 *vp;

    vp = (__u16 *)((__u16)(y * Width) << 1);
    if ((unsigned int)y < MaxRow)
	fmemcpyw(vp, C->vseg, vp + Width, C->vseg, (MaxRow - y) * Width);
    ClearRange(C, 0, MaxRow, MaxCol, MaxRow);
}

#if defined (CONFIG_EMUL_VT52) || defined (CONFIG_EMUL_ANSI)
static void ScrollDown(register Console * C, int y)
{
    register __u16 *vp;
    int yy = MaxRow;

    vp = (__u16 *)((__u16)(yy * Width) << 1);
    while (--yy >= y) {
	fmemcpyw(vp, C->vseg, vp - Width, C->vseg, Width);
	vp -= Width;
    }
    ClearRange(C, 0, y, MaxCol, y);
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

    SetDisplayPage(Visible);
    PositionCursor(Visible);
    Current_VCminor = N;
}

struct tty_ops dircon_ops = {
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
    unsigned PageSizeW;
    unsigned VideoSeg;

    MaxCol = (Width = peekb(0x4a, 0x40)) - 1;  /* BIOS data segment */

    /* Trust this. Cga does not support peeking at 0x40:0x84. */
    MaxRow = (Height = 25) - 1;
    CCBase = (void *) peekw(0x63, 0x40);
    PageSizeW = ((unsigned int)peekw(0x4C, 0x40) >> 1);

    VideoSeg = 0xb800;
    if (peekb(0x49, 0x40) == 7) {
	VideoSeg = 0xB000;
	NumConsoles = 1;
    }

    C = Con;
    Visible = C;

    for (i = 0; i < NumConsoles; i++) {
	C->cx = C->cy = 0;
	if (!i) {
	    C->cx = peekb(0x50, 0x40);
	    C->cy = peekb(0x51, 0x40);
	}
	C->fsm = std_char;
	C->basepage = i * PageSizeW;
	C->vseg = VideoSeg + (C->basepage >> 3);
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

    printk("Direct console, %s kbd %ux%u"TERM_TYPE"(%u virtual consoles)\n",
	   kbd_name, Width, Height, NumConsoles);
}
