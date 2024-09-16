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
#include <linuxmt/debug.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/string.h>
#include <linuxmt/errno.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>
#include <linuxmt/kd.h>
#include <arch/io.h>
#include "console.h"
#include "crtc-6845.h"

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

#ifdef CONFIG_CONSOLE_DUAL
#define MAX_DISPLAYS    2
#else
#define MAX_DISPLAYS    1
#endif

struct console;
typedef struct console Console;

struct console {
    int Width, Height;
    int cx, cy;                 /* cursor position */
    unsigned char display;
    unsigned char type;
    unsigned char attr;         /* current attribute */
    unsigned char XN;           /* delayed newline on column 80 */
    void (*fsm)(Console *, int);
    unsigned int vseg;          /* vram for this console page */
    int vseg_offset;            /* vram offset of vseg for this console page */
    unsigned short crtc_base;   /* 6845 CRTC base I/O address */
#ifdef CONFIG_EMUL_ANSI
    int savex, savey;           /* saved cursor position */
    unsigned char *parmptr;     /* ptr to params */
    unsigned char params[MAXPARMS];     /* ANSI params */
#endif
};

static Console *glock;
static struct wait_queue glock_wait;
static Console *Visible[MAX_DISPLAYS];
static Console Con[MAX_CONSOLES];
static int NumConsoles;

unsigned int VideoSeg = 0xb800;
int Current_VCminor;
int kraw;

#ifdef CONFIG_EMUL_ANSI
#define TERM_TYPE " emulating ANSI "
#else
#define TERM_TYPE " dumb "
#endif

static void std_char(register Console *, int);

static void SetDisplayPage(register Console * C)
{
    outw((C->vseg_offset & 0xff00) | 0x0c, C->crtc_base);
    outw((C->vseg_offset << 8) | 0x0d, C->crtc_base);
}

static void PositionCursor(register Console * C)
{
    unsigned int Pos = C->cx + C->Width * C->cy + C->vseg_offset;

    outb(14, C->crtc_base);
    outb(Pos >> 8, C->crtc_base + 1);
    outb(15, C->crtc_base);
    outb(Pos, C->crtc_base + 1);
}

static void DisplayCursor(Console * C, int onoff)
{
    /* unfortunately, the cursor start/end at BDA 0x0460 can't be relied on! */
    unsigned int v;

    if (onoff)
        v = C->type == OT_MDA ? 0x0b0c : (C->type == OT_CGA ? 0x0607: 0x0d0e);
    else v = 0x2000;

    outb(10, C->crtc_base);
    outb(v >> 8, C->crtc_base + 1);
    outb(11, C->crtc_base);
    outb(v, C->crtc_base + 1);
}

static void VideoWrite(register Console * C, int c)
{
    pokew((C->cx + C->cy * C->Width) << 1, (seg_t) C->vseg,
          (C->attr << 8) | (c & 255));
}

static void ClearRange(register Console * C, int x, int y, int x2, int y2)
{
    int vp;

    x2 = x2 - x + 1;
    vp = (x + y * C->Width) << 1;
    do {
        for (x = 0; x < x2; x++) {
            pokew(vp, (seg_t) C->vseg, (C->attr << 8) | ' ');
            vp += 2;
        }
        vp += (C->Width - x2) << 1;
    } while (++y <= y2);
}

static void ScrollUp(register Console * C, int y)
{
    int vp;
    int MaxRow = C->Height - 1;
    int MaxCol = C->Width - 1;

    vp = y * (C->Width << 1);
    if ((unsigned int)y < MaxRow)
        fmemcpyw((void *)vp, C->vseg,
                 (void *)(vp + (C->Width << 1)), C->vseg, (MaxRow - y) * C->Width);
    ClearRange(C, 0, MaxRow, MaxCol, MaxRow);
}

#ifdef CONFIG_EMUL_ANSI
static void ScrollDown(register Console * C, int y)
{
    int vp;
    int yy = C->Height - 1;

    vp = yy * (C->Width << 1);
    while (--yy >= y) {
        fmemcpyw((void *)vp, C->vseg, (void *)(vp - (C->Width << 1)), C->vseg, C->Width);
        vp -= C->Width << 1;
    }
    ClearRange(C, 0, y, C->Width - 1, y);
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
    if ((N >= NumConsoles) || glock)
        return;

    Visible[C->display] = C;
    SetDisplayPage(C);
    PositionCursor(C);
    DisplayCursor(&Con[Current_VCminor], 0);
    Current_VCminor = N;
    DisplayCursor(&Con[Current_VCminor], 1);
}

struct tty_ops dircon_ops = {
    Console_open,
    Console_release,
    Console_write,
    NULL,
    Console_ioctl,
    Console_conout
};

#ifndef CONFIG_CONSOLE_DUAL

#ifdef DEBUG
static const char *type_string[] = {
    "MDA",
    "CGA",
    "EGA",
    "VGA",
};
#endif

void INITPROC console_init(void)
{
    Console *C = &Con[0];
    int i;
    int Width, Height;
    unsigned int PageSizeW;
    unsigned short boot_crtc;
    unsigned char output_type = OT_EGA;

    Width = peekb(0x4a, 0x40);  /* BIOS data segment */
    /* Trust this. Cga does not support peeking at 0x40:0x84. */
    Height = 25;
    boot_crtc = peekw(0x63, 0x40);
    PageSizeW = ((unsigned int)peekw(0x4C, 0x40) >> 1);

    NumConsoles = MAX_CONSOLES - 1;
    if (peekb(0x49, 0x40) == 7) {
        VideoSeg = 0xB000;
        NumConsoles = 1;
        output_type = OT_MDA;
    } else {
        if (peekw(0xA8+2, 0x40) == 0)
            output_type = OT_CGA;
    }

    Visible[0] = C;

    for (i = 0; i < NumConsoles; i++) {
        C->cx = C->cy = 0;
        C->display = 0;
        if (!i) {
            C->cx = peekb(0x50, 0x40);
            C->cy = peekb(0x51, 0x40);
        }
        C->fsm = std_char;
        C->vseg_offset = i * PageSizeW;
        C->vseg = VideoSeg + (C->vseg_offset >> 3);
        C->attr = A_DEFAULT;
        C->type = output_type;
        C->Width = Width;
        C->Height = Height;
        C->crtc_base = boot_crtc;

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
#else
void INITPROC console_init(void)
{
    Console *C = &Con[0];
    int i, j, dev;
    unsigned short boot_crtc;
    unsigned char boot_type;
    unsigned char screens = 0;
    unsigned char cur_display = 0;

    boot_crtc = peekw(0x63, 0x40);
    for (i = 0; i < N_DEVICETYPES; ++i) {
        if (crtc_params[i].crtc_base == boot_crtc)
            boot_type = i;
    }
    for (i = 0; i < N_DEVICETYPES; ++i) {
        dev = (i + boot_type) % N_DEVICETYPES;
        if (!crtc_probe(crtc_params[dev].crtc_base))
            continue;
        screens++;
        crtc_init(dev);
        for (j = 0; j < crtc_params[dev].max_pages; ++j) {
            C->cx = C->cy = 0;
            C->display = cur_display;
            if (!j) Visible[C->display] = C;
            if (!j && !i) {
                C->cx = peekb(0x50, 0x40);
                C->cy = peekb(0x51, 0x40);
            }
            C->fsm = std_char;
            C->vseg_offset = j * crtc_params[dev].page_words;
            C->vseg = crtc_params[dev].vseg_base + (C->vseg_offset >> 3);
            C->attr = A_DEFAULT;
            C->type = dev;
            C->Width = crtc_params[dev].w;
            C->Height = crtc_params[dev].h;
            C->crtc_base = crtc_params[dev].crtc_base;
#ifdef CONFIG_EMUL_ANSI
            C->savex = C->savey = 0;
#endif
            NumConsoles++;
            if (i) DisplayCursor(C, 0);
            C++;
        }
        cur_display++;
    }

    /* For kernel/timer.c */
    VideoSeg = Visible[0]->vseg;

    kbd_init();
    printk("Direct console %s kbd"TERM_TYPE"(%d screens, %i consoles)\n",
        kbd_name, screens, NumConsoles);
    for (i = 0; i < NumConsoles; ++i) {
        debug("/dev/tty%i, %s, %ux%u\n", i + 1, type_string[Con[i].type], Con[i].Width, Con[i].Height);
    }
}
#endif
