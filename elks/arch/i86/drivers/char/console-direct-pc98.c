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
 *
 * Modified for PC-98
 * T. Yamada 2021
 *
 */

#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/string.h>
#include <linuxmt/errno.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>
#include <linuxmt/kd.h>
#include <arch/io.h>
#include "console.h"
#include "conio-pc98-asm.h"

/* Assumes ASCII values. */
#define isalpha(c) (((unsigned char)(((c) | 0x20) - 'a')) < 26)

#define A_DEFAULT 	0x07
#define A_BOLD 		0x08
#define A_UNDERLINE 	0x07
#define A_BLINK 	0x80
#define A_REVERSE	0x70
#define A_BLANK		0x00

#define A98_DEFAULT	0xE1

/* character definitions*/
#define BS		'\b'
#define NL		'\n'
#define CR		'\r'
#define TAB		'\t'
#define ESC		'\x1B'
#define BEL		'\x07'

#define MAXPARMS	28

#define MAX_DISPLAYS 1

struct console;
typedef struct console Console;

struct console {
    int Width, Height;
    int cx, cy;			/* cursor position */
    unsigned char display;
    unsigned char unused;
    unsigned char attr;		/* current attribute */
    unsigned char XN;		/* delayed newline on column 80 */
    void (*fsm)(Console *, int);
    unsigned int vseg;		/* vram for this console page */
    unsigned int vseg_offset;	/* vram offset of vseg for this console page */
#ifdef CONFIG_EMUL_ANSI
    int savex, savey;		/* saved cursor position */
    unsigned char *parmptr;	/* ptr to params */
    unsigned char params[MAXPARMS];	/* ANSI params */
#endif
};

static Console *glock;
static struct wait_queue glock_wait;
static Console *Visible[MAX_DISPLAYS];
static Console Con[MAX_CONSOLES];
static int NumConsoles = MAX_CONSOLES;

int Current_VCminor;
int kraw;
unsigned VideoSeg;
unsigned AttributeSeg;

#ifdef CONFIG_EMUL_ANSI
#define TERM_TYPE " emulating ANSI "
#else
#define TERM_TYPE " dumb "
#endif

static void std_char(Console *, int);

static void SetDisplayPage(register Console * C)
{
}

static void PositionCursor(register Console * C)
{
    int Pos;

    Pos = C->cx + C->Width * C->cy + C->vseg_offset;
    cursor_set(Pos * 2);
}

static void DisplayCursor(Console * C, int onoff)
{
    if (onoff)
	cursor_on();
    else
	cursor_off();
}

static word_t conv_pcattr(word_t attr)
{
    static unsigned char grb[8] = {0x00, 0x20, 0x80, 0xA0, 0x40, 0x60, 0xC0, 0xE0};

    word_t attr98;
    unsigned char fg_grb;
    unsigned char bg_grb;

    fg_grb = grb[attr & 0x7];
    bg_grb = grb[(attr & 0x70) >> 4];

    if ((fg_grb == 0) && (bg_grb == 0))
	attr98 = fg_grb;        /* No display */
    else if (bg_grb != 0)
	attr98 = 0x05 | bg_grb; /* Use bg color and invert */
    else
	attr98 = 0x01 | fg_grb; /* Use fg color */

    return attr98;
}

static void VideoWrite(register Console * C, int c)
{
    word_t addr;
    word_t attr;

    addr = (C->cx + C->cy * C->Width) << 1;
    attr = (C->attr == A_DEFAULT) ? A98_DEFAULT : conv_pcattr(C->attr);

    pokew(addr, (seg_t) AttributeSeg, attr);
    pokew(addr, (seg_t) C->vseg, c & 255);
}

static void ClearRange(register Console * C, int x, int y, int xx, int yy)
{
    __u16 *vp;
    word_t attr;

    attr = (C->attr == A_DEFAULT) ? A98_DEFAULT : conv_pcattr(C->attr);

    xx = xx - x + 1;
    vp = (__u16 *)((__u16)(x + y * C->Width) << 1);
    do {
	for (x = 0; x < xx; x++) {
	    pokew((word_t) vp, AttributeSeg, attr);
	    pokew((word_t) (vp++), (seg_t) C->vseg, (word_t) ' ');
	}
	vp += (C->Width - xx);
    } while (++y <= yy);
}

static void ScrollUp(register Console * C, int y)
{
    __u16 *vp;
    int MaxRow = C->Height - 1;
    int MaxCol = C->Width - 1;

    vp = (__u16 *)((__u16)(y * C->Width) << 1);
    if ((unsigned int)y < MaxRow) {
	fmemcpyb(vp, AttributeSeg, vp + C->Width, AttributeSeg, (MaxRow - y) * (C->Width << 1));
	fmemcpyb(vp, C->vseg, vp + C->Width, C->vseg, (MaxRow - y) * (C->Width << 1));
    }
    ClearRange(C, 0, MaxRow, MaxCol, MaxRow);
}

#ifdef CONFIG_EMUL_ANSI
static void ScrollDown(register Console * C, int y)
{
    __u16 *vp;
    int MaxRow = C->Height - 1;
    int MaxCol = C->Width - 1;
    int yy = MaxRow;

    vp = (__u16 *)((__u16)(yy * C->Width) << 1);
    while (--yy >= y) {
	fmemcpyb(vp, AttributeSeg, vp - C->Width, AttributeSeg, C->Width << 1);
	fmemcpyb(vp, C->vseg, vp - C->Width, C->vseg, C->Width << 1);
	vp -= C->Width;
    }
    ClearRange(C, 0, y, MaxCol, y);
}
#endif

/* shared console routines*/
#include "console.c"

/* This also tells the keyboard driver which tty to direct it's output to...
 * CAUTION: It *WILL* break if the console driver doesn't get tty0-X.
 */

void Console_set_vc(int N)
{
    Console *C = &Con[N];
    if ((N >= NumConsoles) || (Visible[C->display] == C) || glock)
        return;
    Visible[C->display] = C;
    SetDisplayPage(Visible[C->display]);
    PositionCursor(Visible[C->display]);
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

void INITPROC console_init(void)
{
    Console *C = &Con[0];
    int i;
    unsigned PageSizeW;

    VideoSeg = 0xA000;
    AttributeSeg = 0xA200;

    if (peekb(0x501,0) & 8) { /* High Resolution PC-98 */
	VideoSeg = 0xE000;
	AttributeSeg = 0xE200;
    }

    C->Width = 80;
    C->Height = 25;

    PageSizeW = 2000;

    NumConsoles = 1;

    for (i = 0; i < NumConsoles; i++) {
    C->display = 0;
	C->cx = C->cy = 0;
	if (!i) {
	    Visible[C->display] = C;
	    C->cx = read_tvram_x() % 160;
	    C->cy = read_tvram_x() / 160;
	}
	C->fsm = std_char;
	C->vseg_offset = i * PageSizeW;
	C->vseg = VideoSeg + (C->vseg_offset >> 3);
	C->attr = A_DEFAULT;

#ifdef CONFIG_EMUL_ANSI

	C->savex = C->savey = 0;

#endif

	/* Do not erase early printk() */
	/* ClearRange(C, 0, C->cy, MaxCol, MaxRow); */

	C++;
    }

    kbd_init();

    printk("Direct console, %s kbd %ux%u"TERM_TYPE"(du virtual consoles)\n",
	   kbd_name, Con[0].Width, Con[0].Height, NumConsoles);
}
