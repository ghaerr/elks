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
#include <arch/swan.h>
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

#define MAX_DISPLAYS    1

#ifdef CONFIG_CONSOLE_FONT_4X8
#define HALFWIDTH_FONT
#endif

struct console;
typedef struct console Console;

struct console {
    int Width, Height;
    int cx, cy;                 /* cursor position */
    unsigned char display;
    unsigned char sy;           /* scroll Y position */
    unsigned char attr;         /* current attribute */
    unsigned char XN;           /* delayed newline on column 80 */
    void (*fsm)(Console *, int);
    unsigned int font_ofs;      /* font offset */
    unsigned int vseg;       /* screen segment */
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

int Current_VCminor;
int kraw;

#ifdef CONFIG_EMUL_ANSI
#define TERM_TYPE " emulating ANSI "
#else
#define TERM_TYPE " dumb "
#endif

static void std_char(Console *, int);

static void SetDisplayPage(Console * C)
{
    outw(0x0000, SCR_CONTROL_PORT);
    outb(((C->vseg >> 7) * 0x11) + 0x01, SCR_BASE_PORT);
    outw(((unsigned int) C->sy) << 11, SCR1_SCROLL_PORT);
#ifdef HALFWIDTH_FONT
    outw((((unsigned int) C->sy) << 11) | 0x04, SCR2_SCROLL_PORT);
    outb(0x03, SCR_CONTROL_PORT);
#else
    outb(0x01, SCR_CONTROL_PORT);
#endif
}

static void PositionCursor(Console * C)
{
    /* TODO */
}

static void DisplayCursor(Console * C, int onoff)
{
    /* TODO */
}

static void VideoWrite(Console * C, int c)
{
#ifdef HALFWIDTH_FONT
    pokew(((C->cx >> 1) + ((C->cy + C->sy) & 31) * 32) << 1,
          (seg_t) C->vseg + ((C->cx & 1) << 7),
          C->font_ofs | ((C->attr & 0x0F) << 9) | (c & 255));
#else
    pokew((C->cx + ((C->cy + C->sy) & 31) * 32) << 1, (seg_t) C->vseg,
          C->font_ofs | ((C->attr & 0x0F) << 9) | (c & 255));
#endif
}

static void ClearRange(Console * C, int x, int y, int x2, int y2)
{
#ifdef HALFWIDTH_FONT
    int i;
#endif
    int vp;

    y += (C->sy & 31);
    y2 += (C->sy & 31);

    x2 = x2 - x + 1;
#ifdef HALFWIDTH_FONT
    /* TODO: Optimize */
    vp = (y * 32) << 1;
    do {
        vp &= 0x7FF;
        for (i = x; i <= x2; i++)
            pokew(vp + (i & ~1), (seg_t) C->vseg + ((i & 1) << 7), C->font_ofs);
        vp += 64;
    } while (++y <= y2);
#else
    vp = (x + y * 32) << 1;
    do {
        vp &= 0x7FF;
        for (x = 0; x < x2; x++) {
            pokew(vp, (seg_t) C->vseg, C->font_ofs);
            vp += 2;
        }
        vp += (32 - x2) << 1;
    } while (++y <= y2);
#endif
}

static void ScrollUp(Console * C, int y)
{
    int vp;
    int MaxRow = C->Height - 1;
    int MaxCol = C->Width - 1;

    if (y == 0) {
        /* Fast path - use hardware scroll */
        ClearRange(C, 0, MaxRow + 1, MaxCol, MaxRow + 1);

        C->sy++;
        SetDisplayPage(C);
        return;
    }

    vp = y * 64;
    if ((unsigned int)y < MaxRow) {
        fmemcpyw((void *)vp, C->vseg,
                 (void *)(vp + 64), C->vseg, (MaxRow - y) * 32);
#ifdef HALFWIDTH_FONT
        fmemcpyw((void *)vp, C->vseg + 0x80,
                 (void *)(vp + 64), C->vseg + 0x80, (MaxRow - y) * 32);
#endif
    }
    ClearRange(C, 0, MaxRow, MaxCol, MaxRow);
}

#ifdef CONFIG_EMUL_ANSI
static void ScrollDown(Console * C, int y)
{
    int vp;
    int yy = C->Height - 1;

    vp = yy * 64;
    while (--yy >= y) {
        fmemcpyw((void *)vp, C->vseg, (void *)(vp - 64), C->vseg, C->Width);
#ifdef HALFWIDTH_FONT
        fmemcpyw((void *)vp, C->vseg + 0x80, (void *)(vp - 64), C->vseg + 0x80, C->Width);
#endif
        vp -= 64;
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
    /* PositionCursor(C);
    DisplayCursor(&Con[Current_VCminor], 0); */
    Current_VCminor = N;
    /* DisplayCursor(&Con[Current_VCminor], 1); */
}

struct tty_ops dircon_ops = {
    Console_open,
    Console_release,
    Console_write,
    NULL,
    Console_ioctl,
    Console_conout
};

extern unsigned int font_4x8[];
extern unsigned int font_8x8[];

#define RGB4(r,g,b) (((r) << 8) | ((g) << 4) | (b))
static unsigned int color_palette[16] = {
    RGB4( 0, 0, 0),
    RGB4( 0, 0,10),
    RGB4( 0,10, 0),
    RGB4( 0,10,10),
    RGB4(10, 0, 0),
    RGB4(10, 0,10),
    RGB4(10, 5, 0),
    RGB4(10,10,10),
    RGB4( 5, 5, 5),
    RGB4( 5, 5,15),
    RGB4( 5,15, 5),
    RGB4( 5,15,15),
    RGB4(15, 5, 5),
    RGB4(15, 5,15),
    RGB4(15,15, 5),
    RGB4(15,15,15)
};

static unsigned int addr_to_tile(unsigned int addr)
{
    if (addr > 0x4000)
        return ((addr - 0x4000) >> 4) | (1 << 13);
    else
        return ((addr - 0x2000) >> 4);
}

void INITPROC console_init(void)
{
    Console *C = &Con[0];
    int i;
    unsigned int low_mem_ofs;
    unsigned int font_ofs;

    /* Set palette */
    for (i = 0; i < 16; i++) {
        pokew(0xFE00 + (i << 1), (seg_t) 0, color_palette[i]);
        pokew(0xFE02 + (i << 5), (seg_t) 0, color_palette[i]);
    }

    /* Allocate room for font */
#if defined(CONFIG_CONSOLE_FONT_4X8)
    outb(inb(0x60) | 0xC0, 0x60);

    low_mem_ofs = 0x8000;
    low_mem_ofs -= (256 * 32);
    font_ofs = addr_to_tile(low_mem_ofs >> 1);

    unsigned long __far *dest = _MK_FP(0, low_mem_ofs);
    unsigned char __far *src = _MK_FP(kernel_cs, font_4x8);
    for (i = 0; i < 256 * 4; i++) {
        *(dest++) = *src & 0x0F;
        *(dest++) = *(src++) >> 4;
    }
#elif defined(CONFIG_CONSOLE_FONT_8X8)
    low_mem_ofs = 0x6000;
    low_mem_ofs -= (256 * 16);
    font_ofs = addr_to_tile(low_mem_ofs);

    unsigned short __far *dest = _MK_FP(0, low_mem_ofs);
    unsigned char __far *src = _MK_FP(kernel_cs, font_8x8);
    for (i = 0; i < 256 * 8; i++)
        *(dest++) = *(src++);
#else
#error No font defined!
#endif

    NumConsoles = MAX_CONSOLES - 1;

    for (i = 0; i < NumConsoles; i++) {
        /* Allocate room for console data */
#ifdef HALFWIDTH_FONT
        low_mem_ofs -= (32 * 32 * 4);
#else
        low_mem_ofs -= (32 * 32 * 2);
#endif

        C->cx = C->cy = 0;
        C->sy = 0;
        C->display = 0;
        C->fsm = std_char;
        C->vseg = (low_mem_ofs >> 4);
        C->font_ofs = font_ofs;
        C->attr = A_DEFAULT;
#ifdef HALFWIDTH_FONT
        C->Width = 56;
#else
        C->Width = 28;
#endif
        C->Height = 18;

#ifdef CONFIG_EMUL_ANSI
        C->savex = C->savey = 0;
#endif

#ifdef HALFWIDTH_FONT
        ClearRange(C, 0, 0, 57, 31);
#else
        ClearRange(C, 0, 0, 27, 31);
#endif

        C++;
    }

    Console_set_vc(0);

    kbd_init();

    printk("Direct console, %s kbd %ux%u"TERM_TYPE"(%d virtual consoles)\n",
           kbd_name, Visible[0]->Width, Visible[0]->Height, NumConsoles);
}
