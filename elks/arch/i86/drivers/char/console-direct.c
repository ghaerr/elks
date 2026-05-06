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
    unsigned int vseg;          /* current render target: video page seg when foreground,
                                   ram_buf_seg when backgrounded in RAM-buffer mode */
    unsigned int vseg_offset;   /* CRTC start address (chars) for this VC's video page;
                                   only meaningful when this VC is video-page-backed */
    unsigned short crtc_base;   /* 6845 CRTC base I/O address */
    unsigned char *ram_buf;     /* heap_alloc'd backing buffer (RAM-buffer mode);
                                   NULL if this VC is video-page-backed */
    unsigned int ram_buf_seg;   /* paragraph-aligned segment alias for ram_buf */
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
/* When non-zero, MAX_CONSOLES exceeds the number of video text pages this
 * adapter can back natively. In that mode every VC has a RAM backing buffer;
 * Console_set_vc() blits buffer<->video on switch instead of just flipping
 * the CRTC start address. Single-display only; CONFIG_CONSOLE_DUAL retains
 * pure page-flip behavior bounded by hardware page count. */
static int g_use_rambuf;

unsigned int VideoSeg = 0xb800;
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
    outw((C->vseg_offset & 0xff00) | 0x0c, C->crtc_base);
    outw((C->vseg_offset << 8) | 0x0d, C->crtc_base);
}

static void PositionCursor(Console * C)
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

static void VideoWrite(Console * C, int c)
{
    pokew((C->cx + C->cy * C->Width) << 1, (seg_t) C->vseg,
          (C->attr << 8) | (c & 255));
}

static void ClearRange(Console * C, int x, int y, int x2, int y2)
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

static void ScrollUp(Console * C, int y)
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
static void ScrollDown(Console * C, int y)
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

/* Enable graphics-mode text page save/restore in the shared Console_ioctl().
 * Only the IBM-PC direct driver provides the heap_alloc, kernel_ds, fmemcpyw
 * and vseg ingredients the save/restore relies on. Other CONFIG_CONSOLE_DIRECT
 * variants (PC-98) define a different Console struct and skip it. */
#define VC_GRAPH_SAVE_RESTORE 1

/* shared console routines*/
#include "console.c"

/* This also tells the keyboard driver which tty to direct it's output to...
 * CAUTION: It *WILL* break if the console driver doesn't get tty0-X.
 */

void Console_set_vc(int N)
{
    Console *C = &Con[N];
    Console *outgoing;
    if ((N >= NumConsoles) || glock)
        return;

    outgoing = Visible[C->display];
    if (g_use_rambuf) {
        /* RAM-buffer mode: foreground VC writes go to VideoSeg directly;
         * backgrounded VCs write to their ram_buf_seg. Swap on transition. */
        if (outgoing && outgoing != C) {
            /* save outgoing's screen into its RAM buffer, retarget its writes there */
            fmemcpyw((void *)0, (seg_t) outgoing->ram_buf_seg,
                     (void *)0, (seg_t) outgoing->vseg,
                     outgoing->Width * outgoing->Height);
            outgoing->vseg = outgoing->ram_buf_seg;
            /* restore incoming's screen from its RAM buffer, retarget writes to video */
            fmemcpyw((void *)0, (seg_t) VideoSeg,
                     (void *)0, (seg_t) C->ram_buf_seg,
                     C->Width * C->Height);
            C->vseg = VideoSeg;
        }
    } else {
        SetDisplayPage(C);
    }
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
static unsigned char INITPROC pages_for_type(unsigned char t)
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
    int j;
    int Width, Height;
    unsigned int PageSizeW;
    unsigned int bufsize;
    unsigned int linear_off;
    unsigned char *raw;
    unsigned char avail_pages;
    unsigned short boot_crtc;
    unsigned char output_type = OT_EGA;

    Width = peekb(0x4a, 0x40);  /* BIOS data segment */
    /* Trust this. Cga does not support peeking at 0x40:0x84. */
    Height = 25;
    boot_crtc = peekw(0x63, 0x40);
    PageSizeW = ((unsigned int)peekw(0x4C, 0x40) >> 1);

    if (peekb(0x49, 0x40) == 7) {
        VideoSeg = 0xB000;
        output_type = OT_MDA;
    } else {
        if (peekw(0xA8+2, 0x40) == 0)
            output_type = OT_CGA;
    }
    avail_pages = pages_for_type(output_type);

    NumConsoles = MAX_CONSOLES;
    /* Kernel built for more VCs than the adapter can back.  
     * Alloc paragraph-aligned RAM buffers up front.
     * Failure rolls back and clamps to avail_pages (pure video-page mode). */
    if (NumConsoles > avail_pages) {
        int alloc_ok = 1;
        bufsize = Width * Height * 2;
        for (j = 0; j < NumConsoles; j++) {
            raw = heap_alloc(bufsize + 15, HEAP_TAG_DRVR | HEAP_TAG_CLEAR);
            if (!raw) {
                alloc_ok = 0;
                break;
            }
            Con[j].ram_buf = raw;
            linear_off = ((unsigned int)raw + 15) & ~0xF;
            Con[j].ram_buf_seg = kernel_ds + (linear_off >> 4);
        }
        if (alloc_ok) {
            g_use_rambuf = 1;
        } else {
            while (--j >= 0) {
                heap_free(Con[j].ram_buf);
                Con[j].ram_buf = NULL;
                Con[j].ram_buf_seg = 0;
            }
            NumConsoles = avail_pages;
            printk("console: RAM buffer alloc failed, clamping to %d VCs\n",
                   NumConsoles);
        }
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
        if (g_use_rambuf) {
            /* Foreground VC writes go to video RAM, 
             * backgrounded VCs to their own buffers. 
             * SetDisplayPage() is unused in this mode -
             * only one video page is ever displayed. */
            C->vseg_offset = 0;
            C->vseg = (i == 0) ? VideoSeg : C->ram_buf_seg;
        } else {
            C->vseg_offset = i * PageSizeW;
            C->vseg = VideoSeg + (C->vseg_offset >> 3);
        }
        C->attr = A_DEFAULT;
        C->type = output_type;
        C->Width = Width;
        C->Height = Height;
        C->crtc_base = boot_crtc;

#ifdef CONFIG_EMUL_ANSI
        C->savex = C->savey = 0;
#endif

        /* Background VCs in RAM-buffer mode start with cleared screens.
         * The foreground VC's video memory is left alone to preserve 
         * printk() output.  */
        if (g_use_rambuf && i > 0)
            ClearRange(C, 0, 0, Width - 1, Height - 1);

        C++;
    }

    kbd_init();

    printk("Direct console, %s kbd %ux%u"TERM_TYPE"(%d virtual consoles%s)\n",
           kbd_name, Width, Height, NumConsoles,
           g_use_rambuf ? ", RAM-buffered" : "");
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
    printk("Direct console %s kbd"TERM_TYPE"(%d screens, %d consoles)\n",
        kbd_name, screens, NumConsoles);
    for (i = 0; i < NumConsoles; ++i) {
        debug("/dev/tty%d, %s, %ux%u\n", i + 1, type_string[Con[i].type], Con[i].Width, Con[i].Height);
    }
}
#endif
