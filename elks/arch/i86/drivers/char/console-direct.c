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
#include <linuxmt/heap.h>
#include <arch/io.h>
#include <arch/segment.h>
#include "console.h"
#include "crtc-6845.h"

/* Enable ram-buffered text pages */
#define VC_USE_RAM_BUFFER     0

/* Enable graphics-mode text page save/restore in the shared Console_ioctl() */
#define VC_GRAPH_SAVE_RESTORE 0

#ifdef CONFIG_CONSOLE_DUAL
#define MAX_DISPLAYS    2
#else
#define MAX_DISPLAYS    1
#endif

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

struct console {
    int Width, Height;
    int cx, cy;                 /* cursor position */
    unsigned char display;
    unsigned char type;
    unsigned char attr;         /* current attribute */
    unsigned char XN;           /* delayed newline on column 80 */
    void (*fsm)(struct console *, int);
    seg_t vseg;                 /* current render target: video page seg when foreground,
                                   ram_seg when backgrounded in RAM-buffer mode */
    segoff_t vseg_offset;       /* offset into vseg for buffer ram calculations */
    unsigned int crtc_offset;   /* CRTC start address (chars) for this VC's video page;
                                   only meaningful when this VC is video-page-backed */
    unsigned short crtc_base;   /* 6845 CRTC base I/O address */
    seg_t ram_seg;              /* seg_alloc'd back buffer segment (RAM-buffer mode) */
#ifdef CONFIG_EMUL_ANSI
    int savex, savey;           /* saved cursor position */
    unsigned char *parmptr;     /* ptr to params */
    unsigned char params[MAXPARMS];     /* ANSI params */
#endif
};
typedef struct console Console;

static Console *glock;
static struct wait_queue glock_wait;
static Console *Visible[MAX_DISPLAYS];
static Console Con[MAX_CONSOLES];
static int NumConsoles;
/* When non-zero, MAX_CONSOLES exceeds the number of video text pages this
 * adapter can back natively. In that mode every VC has a RAM backing buffer;
 * Console_set_vc() blits buffer<->video on switch instead of just flipping
 * the CRTC start address. Single-display only; CONFIG_CONSOLE_DUAL retains
 * pure page-flip behavior bounded by hardware page count.
 */
static char UseRambuf;

unsigned int VideoSeg = VIDEOSEG;
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
    outw((C->crtc_offset & 0xff00) | 0x0c, C->crtc_base);
    outw((C->crtc_offset << 8) | 0x0d, C->crtc_base);
}

static void PositionCursor(Console * C)
{
    unsigned int Pos = C->cx + C->Width * C->cy + C->crtc_offset;

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

static void VideoWrite(Console * C, int c)
{
    pokew(((C->cx + C->cy * C->Width) << 1) + C->vseg_offset, C->vseg,
          (C->attr << 8) | (c & 255));
}

static void ClearRange(Console * C, int x, int y, int x2, int y2)
{
    int vp;

    x2 = x2 - x + 1;
    vp = ((x + y * C->Width) << 1) + C->vseg_offset;
    do {
        for (x = 0; x < x2; x++) {
            pokew(vp, C->vseg, (C->attr << 8) | ' ');
            vp += 2;
        }
        vp += (C->Width - x2) << 1;
    } while (++y <= y2);
}

static void ScrollUp(Console * C, int y)
{
    int vp;
    int MaxRow = C->Height - 1;
    int MaxCol = C->Width - 1;

    vp = (y * (C->Width << 1)) + C->vseg_offset;
    if ((unsigned int)y < MaxRow)
        fmemcpyw((void *)vp, C->vseg,
                 (void *)(vp + (C->Width << 1)), C->vseg, (MaxRow - y) * C->Width);
    ClearRange(C, 0, MaxRow, MaxCol, MaxRow);
}

#ifdef CONFIG_EMUL_ANSI
static void ScrollDown(Console * C, int y)
{
    int vp;
    int yy = C->Height - 1;

    vp = (yy * (C->Width << 1)) + C->vseg_offset;
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

#if VC_USE_RAM_BUFFER
    Console *outgoing = Visible[C->display];
    if (UseRambuf) {
        /* RAM-buffer mode: foreground VC writes go to VideoSeg directly;
         * backgrounded VCs write to their ram_seg. Swap on transition. */
        if (outgoing && outgoing != C) {
            /* save outgoing's screen into its RAM buffer, retarget its writes there */
            fmemcpyw(0, outgoing->ram_seg, 0, outgoing->vseg,
                     outgoing->Width * outgoing->Height);
            outgoing->vseg = outgoing->ram_seg;
            /* restore incoming's screen from its RAM buffer, retarget writes to video */
            fmemcpyw(0, VideoSeg, 0, C->ram_seg, C->Width * C->Height);
            C->vseg = VideoSeg;
        }
    } else  /* fall through #endif */
#endif
        SetDisplayPage(C);
    Visible[C->display] = C;
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

/* Number of native text pages each adapter type can hold in its video RAM. */
static int INITPROC pages_for_type(int t)
{
    switch (t) {
    case OT_MDA: return 1;      /* 4 KB at 0xB000 holds one 80x25 page */
    case OT_CGA: return 4;      /* 16 KB at 0xB800 holds four pages */
    default:     return 8;      /* EGA/VGA, 32 KB holds eight pages */
    }
}

void INITPROC console_init(void)
{
    Console *C = &Con[0];
    int i;
    int Width, Height;
    unsigned int PageSizeW;
    unsigned short boot_crtc;
    int output_type = OT_EGA;
    int avail_pages;

    Width = peekb(0x4a, BIOSSEG);  /* BIOS data segment */
    /* Trust this. Cga does not support peeking at 0x40:0x84. */
    Height = 25;
    boot_crtc = peekw(0x63, BIOSSEG);
    PageSizeW = ((unsigned int)peekw(0x4C, BIOSSEG) >> 1);

    if (peekb(0x49, BIOSSEG) == 7) {
        VideoSeg = 0xB000;          // FIXME MDA needs selector in PMODE
        output_type = OT_MDA;
    } else {
        if (peekw(0xA8+2, BIOSSEG) == 0)
            output_type = OT_CGA;
    }
    NumConsoles = MAX_CONSOLES;
    avail_pages = pages_for_type(output_type);

    /* Kernel built for more VCs than the adapter can back.  
     * Alloc paragraph-aligned RAM buffers up front.
     * Failure rolls back and clamps to avail_pages (pure video-page mode).
     */
    if (NumConsoles > avail_pages) {
#if VC_USE_RAM_BUFFER
        int j;
        int alloc_ok = 1;
        unsigned int bufsize = Width * Height * 2;
        segment_s *seg[8];

        for (j = 0; j < NumConsoles; j++) {
            seg[j] = seg_alloc((bufsize + 15) >> 4, SEG_FLAG_VIDBUF);
            if (!seg[j]) {
                alloc_ok = 0;
                break;
            }
            Con[j].ram_seg = seg[j]->base;
        }
        if (alloc_ok) {
            UseRambuf = 1;
        } else {
            while (--j >= 0) {
                seg_free(seg[j]);
                //Con[j].ram_seg = 0;
            }
            NumConsoles = avail_pages;
        }
#else
        NumConsoles = avail_pages;
#endif
    }
    Visible[0] = C;

    for (i = 0; i < NumConsoles; i++) {
        C->cx = C->cy = 0;
        C->display = 0;
        if (!i) {
            C->cx = peekb(0x50, BIOSSEG);
            C->cy = peekb(0x51, BIOSSEG);
        }
        C->fsm = std_char;
#if VC_USE_RAM_BUFFER
        if (UseRambuf) {
            /* Foreground VC writes go to video RAM, 
             * backgrounded VCs to their own buffers. 
             * SetDisplayPage() is unused in this mode -
             * only one video page is ever displayed.
             */
            C->crtc_offset = 0;
            C->vseg_offset = 0;
            C->vseg = (i == 0) ? VideoSeg : C->ram_seg;
        } else /* fall through #endif */
#endif
        {
            C->crtc_offset = i * PageSizeW;
            C->vseg_offset = C->crtc_offset << 1;
            C->vseg = VideoSeg;
        }
        C->attr = A_DEFAULT;
        C->type = output_type;
        C->Width = Width;
        C->Height = Height;
        C->crtc_base = boot_crtc;

#ifdef CONFIG_EMUL_ANSI
        C->savex = C->savey = 0;
#endif
#if VC_USE_RAM_BUFFER
        /* Background VCs in RAM-buffer mode start with cleared screens.
         * The foreground VC's video memory is left alone to preserve printk() output.
         */
        if (UseRambuf && i != 0)
            ClearRange(C, 0, 0, Width - 1, Height - 1);
#endif
        C++;
    }

    kbd_init();

    printk("Direct console, %s kbd %ux%u"TERM_TYPE"(%d virtual consoles%s)\n",
           kbd_name, Width, Height, NumConsoles,
           UseRambuf ? ", RAM-buffered" : "");
}
#else

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
    int i, j, dev;
    int screens = 0;
    unsigned short boot_crtc;
    unsigned char cur_display = 0;
    unsigned char boot_type;

    boot_crtc = peekw(0x63, BIOSSEG);
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
                C->cx = peekb(0x50, BIOSSEG);
                C->cy = peekb(0x51, BIOSSEG);
            }
            C->fsm = std_char;
            C->crtc_offset = j * crtc_params[dev].page_words;
            C->vseg_offset = C->crtc_offset << 1;
            C->vseg = crtc_params[dev].vseg_base;   // FIXME MDA needs selector in PMODE
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
    printk("Direct console %s kbd"TERM_TYPE"(%d screens, %d consoles)\n",
        kbd_name, screens, NumConsoles);
    for (i = 0; i < NumConsoles; ++i) {
        debug("/dev/tty%d, %s, %ux%u\n", i + 1, type_string[Con[i].type], Con[i].Width, Con[i].Height);
    }
}
#endif
