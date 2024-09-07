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

#define CONFIG_CONSOLE_DUAL

#define CRTC_INDX 0x0
#define CRTC_DATA 0x1
#define CRTC_MODE 0x4
#define CRTC_CSEL 0x5
#define CRTC_STAT 0x6

/* character definitions*/
#define BS              '\b'
#define NL              '\n'
#define CR              '\r'
#define TAB             '\t'
#define ESC             '\x1B'
#define BEL             '\x07'

#define MAXPARMS        28

#define N_DEVICETYPES   2

#ifdef CONFIG_CONSOLE_DUAL
#define MAX_DISPLAYS    2
#else
#define MAX_DISPLAYS    1
#endif

struct console;
typedef struct console Console;

#define OT_MDA 0
#define OT_CGA 1
#define OT_EGA 2
#define OT_VGA 3

static const char *type_string[] = {
    "MDA",
    "CGA",
    "EGA",
    "VGA",
};

struct console {
    int cx, cy;                 /* cursor position */
    void (*fsm)(Console *, int);
    unsigned char display;
    unsigned char type;
    unsigned char attr;         /* current attribute */
    unsigned char XN;           /* delayed newline on column 80 */
    unsigned int vseg;          /* vram for this console page */
    int vseg_offset;            /* vram offset of vseg for this console page */
    unsigned short Width;
    unsigned short MaxCol;
    unsigned short Height;
    unsigned short MaxRow;
    unsigned short crtc_base;   /* 6845 CRTC base I/O address */
#ifdef CONFIG_EMUL_ANSI
    int savex, savey;           /* saved cursor position */
    unsigned char *parmptr;     /* ptr to params */
    unsigned char params[MAXPARMS];     /* ANSI params */
#endif
};

struct hw_params {
    unsigned short w;
    unsigned short h;
    unsigned short crtc_base;
    unsigned vseg_base;
    int vseg_bytes;
    unsigned char init_bytes[16];
    unsigned char n_init_bytes;
    unsigned char max_pages;
    unsigned short page_words;
    unsigned char is_present;
};

static struct hw_params params[N_DEVICETYPES] = {
    { 80, 25, 0x3B4, 0xB000, 0x1000,
        {
            0x61, 0x50, 0x52, 0x0F,
            0x19, 0x06, 0x19, 0x19,
            0x02, 0x0D, 0x0B, 0x0C,
            0x00, 0x00, 0x00, 0x00,
        }, 16, 1, 2000, 0
    }, /* MDA */
    { 80, 25, 0x3D4, 0xB800, 0x4000,
        {
            /* CO80 */
            0x71, 0x50, 0x5A, 0x0A,
            0x1F, 0x06, 0x19, 0x1C,
            0x02, 0x07, 0x06, 0x07,
            0x00, 0x00, 0x00, 0x00,
        }, 16, 3, 2000, 0
    }, /* CGA */
    // TODO
    //{ 0 },                             /* EGA */
    //{ 0 },                             /* VGA */
};

static Console *glock;
static struct wait_queue glock_wait;
static Console *Visible[MAX_DISPLAYS];
static Console Con[MAX_CONSOLES];
static int NumConsoles = 0;

int Current_VCminor = 0;
int kraw = 0;

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
    register int vp;

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
    register int vp;

    vp = y * (C->Width << 1);
    if ((unsigned int)y < C->MaxRow)
        fmemcpyw((void *)vp, C->vseg,
                 (void *)(vp + (C->Width << 1)), C->vseg, (C->MaxRow - y) * C->Width);
    ClearRange(C, 0, C->MaxRow, C->MaxCol, C->MaxRow);
}

#ifdef CONFIG_EMUL_ANSI
static void ScrollDown(register Console * C, int y)
{
    register int vp;
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


/* Check to see if this CRTC is present */
static char probe_crtc(unsigned short crtc_base)
{
    int i;
    unsigned char test = 0x55;
    /* We'll try writing a value to cursor address LSB reg (0x0F), then reading it back */
    outb(0x0F, crtc_base + CRTC_INDX);
    unsigned char original = inb(crtc_base + CRTC_DATA);
    if (original == test) test += 0x10;
    outb(0x0F, crtc_base + CRTC_INDX);
    outb(test, crtc_base + CRTC_DATA);
    /* Now wait a bit */
    for (i = 0; i < 100; ++i) {} // TODO: Verify this doesn't get optimized out
    outb(0x0F, crtc_base + CRTC_INDX);
    unsigned char value = inb(crtc_base + CRTC_DATA);
    if (value != test) return 0;
    outb(0x0F, crtc_base + CRTC_INDX);
    outb(original, crtc_base + CRTC_DATA);
    return 1;
}

static int init_output(unsigned char t)
{
    int i;
    struct hw_params *p = &params[t];
    /* Set 80x25 mode, video off */
    outb(0x01, p->crtc_base + CRTC_MODE);
    /* Program CRTC regs */
    for (i = 0; i < p->n_init_bytes; ++i) {
        outb(i, p->crtc_base + CRTC_INDX);
        outb(p->init_bytes[i], p->crtc_base + CRTC_DATA);
    }

    /* Check & clear vram */
    for (i = 0; i < p->vseg_bytes; i += 2)
        pokew(i, p->vseg_base, 0x5555);
    for (i = 0; i < p->vseg_bytes; i += 2)
        if (peekw(i, p->vseg_base) != 0x5555) return 1;
    for (i = 0; i < p->vseg_bytes; i += 2)
        pokew(i, p->vseg_base, 0x7 << 8 | ' ');

    /* Enable video */
    outb(0x09, p->crtc_base + CRTC_MODE);
    return 0;
}

void INITPROC console_init(void)
{
    Console *C;
    int i, j, dev;
    unsigned short boot_crtc;
    unsigned char boot_type;
    unsigned char screens = 0;
    unsigned char cur_display = 0;
    boot_crtc = peekw(0x63, 0x40);
    for (i = 0; i < N_DEVICETYPES; ++i)
        if (params[i].crtc_base == boot_crtc) boot_type = i;

    C = &Con[0];
    for (i = 0; i < N_DEVICETYPES; ++i) {
        dev = (i + boot_type) % N_DEVICETYPES;
        if (!probe_crtc(params[dev].crtc_base)) continue;
        params[dev].is_present = 1;
        screens++;
        init_output(dev);
        for (j = 0; j < params[dev].max_pages; ++j) {
            C->cx = C->cy = 0;
            C->display = cur_display;
            if (!j) Visible[C->display] = C;
            if (!j && !i) {
                C->cx = peekb(0x50, 0x40);
                C->cy = peekb(0x51, 0x40);
            }
            C->fsm = std_char;
            C->vseg_offset = j * params[dev].page_words;
            C->vseg = params[dev].vseg_base + (C->vseg_offset >> 3);
            C->attr = A_DEFAULT;
            C->type = dev;
            C->Width = params[dev].w;
            C->MaxCol = C->Width - 1;
            C->Height = params[dev].h;
            C->MaxRow = C->Height - 1;
            C->crtc_base = params[dev].crtc_base;
#ifdef CONFIG_EMUL_ANSI
            C->savex = C->savey = 0;
#endif
            NumConsoles++;
            if (i) DisplayCursor(C, 0);
            C++;
        }
        cur_display++;
    }

    kbd_init();
    printk("boot_crtc: %x\n", Con[0].crtc_base);
    printk("Direct console %s kbd"TERM_TYPE"(%d screens):\n", kbd_name, screens);
    for (i = 0; i < NumConsoles; ++i) {
        printk("/dev/tty%i, %s, %ux%u\n", i + 1, type_string[Con[i].type], Con[i].Width, Con[i].Height);
    }
}
