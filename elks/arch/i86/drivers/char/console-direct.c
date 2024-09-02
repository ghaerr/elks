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
#define MAX_DISPLAYS    2

struct output;
typedef struct output Output;
struct console;
typedef struct console Console;

enum OutputType {
    OT_MDA = 0,
    OT_CGA,
    OT_EGA,
    OT_VGA,
};

struct console {
    Output *display;
    int cx, cy;                 /* cursor position */
    void (*fsm)(Console *, int);
    unsigned char attr;         /* current attribute */
    unsigned char XN;           /* delayed newline on column 80 */
    unsigned int vseg;          /* video segment for page TODO: Could we compute this from vseg_base + vseg_offset? */
    int vseg_offset;               /* start of vram for this console */
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
};

static struct hw_params params[N_DEVICETYPES] = {
    { 80, 25, 0x3B4, 0xB000, 0x1000,
        {
            0x61, 0x50, 0x52, 0x0F,
            0x19, 0x06, 0x19, 0x19,
            0x02, 0x0D, 0x0B, 0x0C,
            0x00, 0x00, 0x00, 0x00,
        }, 16, 1, 2000
    }, /* MDA */
    { 80, 25, 0x3D4, 0xB800, 0x4000,
        {
            /* CO80 */
            0x71, 0x50, 0x5A, 0x0A,
            0x1F, 0x06, 0x19, 0x1C,
            0x02, 0x07, 0x06, 0x07,
            0x00, 0x00, 0x00, 0x00,
        }, 16, 3, 2000
    }, /* CGA */
    // TODO
    //{ 0 },                             /* EGA */
    //{ 0 },                             /* VGA */
};

struct output {
    enum OutputType type;
    unsigned short Width;
    unsigned short MaxCol;
    unsigned short Height;
    unsigned short MaxRow;
    unsigned short crtc_base;   /* 6845 CRTC base I/O address */
    unsigned vseg_base;         /* Start of vram for this card */
    Console *consoles[MAX_CONSOLES];
    Console *visible;
    Console *glock;          /* Which console owns the graphics hardware */
    struct wait_queue glock_wait;
    unsigned char n_consoles;
};

static Output Outputs[MAX_DISPLAYS];
static Console Con[MAX_CONSOLES];
// FIXME: We probably need to rework glock logic for multi-console mode
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
    outw((C->vseg_offset & 0xff00) | 0x0c, C->display->crtc_base);
    outw((C->vseg_offset << 8) | 0x0d, C->display->crtc_base);
}

static void PositionCursor(register Console * C)
{
    unsigned int Pos = C->cx + C->display->Width * C->cy + C->vseg_offset;

    outb(14, C->display->crtc_base);
    outb(Pos >> 8, C->display->crtc_base + 1);
    outb(15, C->display->crtc_base);
    outb(Pos, C->display->crtc_base + 1);
}

static void DisplayCursor(Console * C, int onoff)
{
    /* unfortunately, the cursor start/end at BDA 0x0460 can't be relied on! */
    unsigned int v;

    if (onoff)
        v = C->display->type == OT_MDA ? 0x0b0c : (C->display->type == OT_CGA ? 0x0607: 0x0d0e);
    else v = 0x2000;

    outb(10, C->display->crtc_base);
    outb(v >> 8, C->display->crtc_base + 1);
    outb(11, C->display->crtc_base);
    outb(v, C->display->crtc_base + 1);
}

static void VideoWrite(register Console * C, int c)
{
    pokew((C->cx + C->cy * C->display->Width) << 1, (seg_t) C->vseg,
          (C->attr << 8) | (c & 255));
}

static void ClearRange(register Console * C, int x, int y, int x2, int y2)
{
    register int vp;

    x2 = x2 - x + 1;
    vp = (x + y * C->display->Width) << 1;
    do {
        for (x = 0; x < x2; x++) {
            pokew(vp, (seg_t) C->vseg, (C->attr << 8) | ' ');
            vp += 2;
        }
        vp += (C->display->Width - x2) << 1;
    } while (++y <= y2);
}

static void ScrollUp(register Console * C, int y)
{
    register int vp;
    int max_row = C->display->Height - 1;
    int max_col = C->display->Width - 1;

    vp = y * (C->display->Width << 1);
    if ((unsigned int)y < max_row)
        fmemcpyw((void *)vp, C->vseg,
                 (void *)(vp + (C->display->Width << 1)), C->vseg, (max_row - y) * C->display->Width);
    ClearRange(C, 0, max_row, max_col, max_row);
}

#ifdef CONFIG_EMUL_ANSI
static void ScrollDown(register Console * C, int y)
{
    register int vp;
    int yy = C->display->Height - 1;

    vp = yy * (C->display->Width << 1);
    while (--yy >= y) {
        fmemcpyw((void *)vp, C->vseg, (void *)(vp - (C->display->Width << 1)), C->vseg, C->display->Width);
        vp -= C->display->Width << 1;
    }
    ClearRange(C, 0, y, C->display->Width - 1, y);
}
#endif

/* shared console routines*/
#include "console.c"

/* This also tells the keyboard driver which tty to direct it's output to...
 * CAUTION: It *WILL* break if the console driver doesn't get tty0-X.
 */

void Console_set_vc(int N)
{
    if ((N >= NumConsoles) || Con[N].display->glock)
        return;
    Con[N].display->visible = &Con[N];

    SetDisplayPage(Con[N].display->visible);
    PositionCursor(Con[N].display->visible);
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

static void init_output(Output *o)
{
    int i;
    struct hw_params *p = &params[o->type];
    o->Width = p->w;
    o->MaxCol = o->Width - 1;
    o->Height = p->h;
    o->MaxRow = o->Height - 1;
    o->vseg_base = p->vseg_base;
    /* Set 80x25 mode, video off */
    outb(0x01, o->crtc_base + CRTC_MODE);
    /* Program CRTC regs */
    for (i = 0; i < p->n_init_bytes; ++i) {
        outb(i, o->crtc_base + CRTC_INDX);
        outb(p->init_bytes[i], o->crtc_base + CRTC_DATA);
    }

    /* Clear vram */
    for (i = 0; i < p->vseg_bytes; i += 2)
        pokew(i, o->vseg_base, 0x7 << 8 | ' ');

    /* Enable video */
    outb(0x09, o->crtc_base + CRTC_MODE);
}

static const char *type_string(enum OutputType t)
{
    switch (t) {
    case OT_MDA: return "MDA";
    case OT_CGA: return "CGA";
    case OT_EGA: return "EGA";
    case OT_VGA: return "VGA";
    }
}

void INITPROC console_init(void)
{
    Console *C;
    Output *O;
    int i;
    int screens;
    unsigned short boot_crtc;
    // 1. Map out hardware
    for (i = 0; i < N_DEVICETYPES; ++i)
        if (probe_crtc(params[i].crtc_base)) {
            Outputs[i].type = i;
            Outputs[i].crtc_base = params[i].crtc_base;
        }
    /* Ask the BIOS which adapter is selected as the primary one */
    boot_crtc = peekw(0x63, 0x40);
    for (i = 0; i < N_DEVICETYPES; ++i)
        if (boot_crtc == params[i].crtc_base && !Outputs[i].crtc_base)
            panic("boot_crtc not found");
    // 2. Set up hardware
    /* boot_crtc was already set up by bios, but reprogramming it should be fine */
    for (i = 0; i < N_DEVICETYPES; ++i)
        init_output(&Outputs[i]);
    // 3. Map consoles to hardware
    C = &Con[0];
    /* Start with boot crtc */
    for (i = 0; i < N_DEVICETYPES; ++i)
        if (Outputs[i].crtc_base == boot_crtc) O = &Outputs[i];

    for (i = 0; i < params[O->type].max_pages; ++i) {
        C->cx = C->cy = 0;
        if (!i) {
            O->visible = C;
            C->cx = peekb(0x50, 0x40);
            C->cy = peekb(0x51, 0x40);
        }
        C->fsm = std_char;
        C->vseg_offset = i * params[O->type].page_words;
        C->vseg = params[O->type].vseg_base + (C->vseg_offset >> 3);
        C->attr = A_DEFAULT;
        C->display = O;
#ifdef CONFIG_EMUL_ANSI
        C->savex = C->savey = 0;
#endif

        NumConsoles++;
        O->consoles[O->n_consoles++] = C++;
    }
    // FIXME: Remove dumb duplication

    /* Now connect the rest up */
    for (i = 0; i < N_DEVICETYPES; ++i) {
        if (Outputs[i].crtc_base == boot_crtc) continue;
        O = &Outputs[i];
        for (i = 0; i < params[O->type].max_pages; ++i) {
            C->cx = C->cy = 0;
            if (!i) {
                O->visible = C;
            }
            C->fsm = std_char;
            C->vseg_offset = i * params[O->type].page_words;
            C->vseg = params[O->type].vseg_base + (C->vseg_offset >> 3);
            C->attr = A_DEFAULT;
            C->display = O;
    #ifdef CONFIG_EMUL_ANSI
            C->savex = C->savey = 0;
    #endif

            DisplayCursor(C, 0);
            NumConsoles++;
            O->consoles[O->n_consoles++] = C++;
        }
    }
    screens = 0;
    for (i = 0; i < N_DEVICETYPES; ++i)
        if (Outputs[i].crtc_base) screens++;
    kbd_init();
    printk("boot_crtc: %x\n", Con[0].display->crtc_base);
    printk("Direct console %s kbd"TERM_TYPE"(%d screens):\n", kbd_name, screens);
    for (i = 0; i < N_DEVICETYPES; ++i) {
        if (!Outputs[i].crtc_base) continue;
        printk("Screen %d, %d consoles, %s, %ux%u\n", i, Outputs[i].n_consoles,
                type_string(Outputs[i].type), Outputs[i].Width, Outputs[i].Height);
    }
}
