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

/* Assumes ASCII values. */
#define isalpha(c) (((unsigned char)(((c) | 0x20) - 'a')) < 26)

#define A_DEFAULT       0x07
#define A_BOLD          0x08
#define A_UNDERLINE     0x07    /* only works on MDA, normal video on EGA */
#define A_BLINK         0x80
#define A_REVERSE       0x70
#define A_BLANK         0x00

/* character definitions*/
#define BS              '\b'
#define NL              '\n'
#define CR              '\r'
#define TAB             '\t'
#define ESC             '\x1B'
#define BEL             '\x07'

#define MAXPARMS        28

struct console;
typedef struct console Console;

struct console {
    int cx, cy;                 /* cursor position */
    void (*fsm)(Console *, int);
    unsigned char attr;         /* current attribute */
    unsigned char XN;           /* delayed newline on column 80 */
    unsigned int vseg;          /* video segment for page */
    int basepage;               /* start of video ram */
#ifdef CONFIG_EMUL_ANSI
    int savex, savey;           /* saved cursor position */
    unsigned char *parmptr;     /* ptr to params */
    unsigned char params[MAXPARMS];     /* ANSI params */
#endif
};

static struct wait_queue glock_wait;
static Console Con[MAX_CONSOLES], *Visible;
static Console *glock;          /* Which console owns the graphics hardware */
static unsigned short CCBase;   /* 6845 CRTC base I/O address */
static int Width, MaxCol, Height, MaxRow;
static int NumConsoles = MAX_CONSOLES;
static unsigned char isMDA, isCGA;

int Current_VCminor = 0;
int kraw = 0;
unsigned VideoSeg = 0xB800;

#ifdef CONFIG_EMUL_ANSI
#define TERM_TYPE " emulating ANSI "
#else
#define TERM_TYPE " dumb "
#endif

static void std_char(register Console *, int);

static void SetDisplayPage(register Console * C)
{
    outw((C->basepage & 0xff00) | 0x0c, CCBase);
    outw((C->basepage << 8) | 0x0d, CCBase);
}

static void PositionCursor(register Console * C)
{
    unsigned int Pos = C->cx + Width * C->cy + C->basepage;

    outb(14, CCBase);
    outb(Pos >> 8, CCBase + 1);
    outb(15, CCBase);
    outb(Pos, CCBase + 1);
}

static void DisplayCursor(int onoff)
{
    /* unfortunately, the cursor start/end at BDA 0x0460 can't be relied on! */
    unsigned int v;

    if (onoff)
        v = isMDA? 0x0b0c: (isCGA? 0x0607: 0x0d0e);
    else v = 0x2000;

    outb(10, CCBase);
    outb(v >> 8, CCBase + 1);
    outb(11, CCBase);
    outb(v, CCBase + 1);
}

static void VideoWrite(register Console * C, int c)
{
    pokew((C->cx + C->cy * Width) << 1, (seg_t) C->vseg,
          (C->attr << 8) | (c & 255));
}

static void ClearRange(register Console * C, int x, int y, int x2, int y2)
{
    register int vp;

    x2 = x2 - x + 1;
    vp = (x + y * Width) << 1;
    do {
        for (x = 0; x < x2; x++) {
            pokew(vp, (seg_t) C->vseg, (C->attr << 8) | ' ');
            vp += 2;
        }
        vp += (Width - x2) << 1;
    } while (++y <= y2);
}

static void ScrollUp(register Console * C, int y)
{
    register int vp;

    vp = y * (Width << 1);
    if ((unsigned int)y < MaxRow)
        fmemcpyw((void *)vp, C->vseg,
                 (void *)(vp + (Width << 1)), C->vseg, (MaxRow - y) * Width);
    ClearRange(C, 0, MaxRow, MaxCol, MaxRow);
}

#ifdef CONFIG_EMUL_ANSI
static void ScrollDown(register Console * C, int y)
{
    register int vp;
    int yy = MaxRow;

    vp = yy * (Width << 1);
    while (--yy >= y) {
        fmemcpyw((void *)vp, C->vseg, (void *)(vp - (Width << 1)), C->vseg, Width);
        vp -= Width << 1;
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

void INITPROC console_init(void)
{
    Console *C;
    int i;
    unsigned int PageSizeW;

    MaxCol = (Width = peekb(0x4a, 0x40)) - 1;  /* BIOS data segment */

    /* Trust this. Cga does not support peeking at 0x40:0x84. */
    MaxRow = (Height = 25) - 1;
    CCBase = peekw(0x63, 0x40);
    PageSizeW = ((unsigned int)peekw(0x4C, 0x40) >> 1);

    VideoSeg = 0xb800;
    if (peekb(0x49, 0x40) == 7) {
        VideoSeg = 0xB000;
        NumConsoles = 1;
        isMDA = 1;
    } else {
        isCGA = peekw(0xA8+2, 0x40) == 0;
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

#ifdef CONFIG_EMUL_ANSI
        C->savex = C->savey = 0;
#endif

        /* Do not erase early printk() */
        /* ClearRange(C, 0, C->cy, MaxCol, MaxRow); */

        C++;
    }

    kbd_init();

    printk("Direct console, %s kbd %ux%u"TERM_TYPE"(%d virtual consoles)\n",
           kbd_name, Width, Height, NumConsoles);
}
