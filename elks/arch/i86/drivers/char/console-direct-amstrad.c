/*
 * Direct video memory display driver, Amstrad PC1512/PC1640 variant
 *
 * Saku Airila 1996
 * Color originally written by Mbit and Davey
 * Re-wrote for new ntty iface
 * Al Riddoch  1999
 *
 * Rewritten by Greg Haerr <greg@censoft.com> July 1999
 * added reverse video, cleaned up code, reduced size
 * added enough ansi escape sequences for visual editing
 *
 * Amstrad PC1512/PC1640 integrated Paradise PEGA (IGA) support, 2026.
 * Built instead of console-direct.c when CONFIG_CONSOLE_AMSTRAD_IGA is set.
 *
 * The PEGA answers at the CGA/MDA addresses but adds a hardware scroll
 * that must be driven through the 6845 start address, and reports the
 * board's SW10 setting through an extra status bit.  Kept as a separate
 * driver rather than as conditionals in console-direct.c.
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
#include <arch/irq.h>
#include "console.h"
#include "crtc-6845.h"

#ifdef CONFIG_CONSOLE_DUAL
#error CONFIG_CONSOLE_AMSTRAD_IGA does not support CONFIG_CONSOLE_DUAL
#endif

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
    unsigned int crtc_home;     /* home CRTC start (chars) of this VC's page */
    unsigned int page_words;    /* words in this VC's scroll ring, 0=stock path */
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

/*
 * Amstrad PC1512/PC1640 integrated graphics adapter (Paradise PEGA "IGA").
 * The PPI status-1 byte carries a fixed Amstrad fingerprint plus the display
 * default mode switches; the PC1640 option register, read through the
 * printer control port after strobing an option address, exposes SW6-SW10.
 * SW10 set means the internal adapter is disabled by an external card.
 * When active, the driver keeps the BIOS data area coherent and scrolls by
 * moving the CRTC start address through the video bank instead of copying.
 */
#define AMSTRAD_PPI_A           0x60
#define AMSTRAD_PPI_B           0x61
#define AMSTRAD_STATUS1_SELECT  0x80
#define AMSTRAD_STATUS1_FIXED   0x0d
#define AMSTRAD_STATUS1_MASK    0x8d
#define AMSTRAD_STATUS1_DDM     0x30
#define AMSTRAD_STATUS1_DDM_SH  4
#define AMSTRAD_CGA_INDEX       0x3d4
#define AMSTRAD_PRINTER_CTRL    0x37a
#define AMSTRAD_PC1640_OPT      0x20
#define AMSTRAD_SW6             0x40
#define AMSTRAD_SW7             0x80
#define AMSTRAD_SW9_PROBE       0x0278
#define AMSTRAD_SW10_PROBE      0x4278

static unsigned char amstrad_iga_active;
static unsigned char amstrad_iga_status1;
static unsigned char amstrad_iga_ddm;
static unsigned char amstrad_iga_sw6;
static unsigned char amstrad_iga_sw7;
static unsigned char amstrad_iga_sw9;
static unsigned char amstrad_iga_sw10;
extern int amstrad_iga_override; /* /bootopts iga= 1 forces on, 2 forces off */

static unsigned char INITPROC AmstradStatus1(void)
{
    unsigned int flags;
    unsigned char portb;
    unsigned char status;

    save_flags(flags);
    clr_irq();
    portb = inb(AMSTRAD_PPI_B);
    outb(portb | AMSTRAD_STATUS1_SELECT, AMSTRAD_PPI_B);
    status = inb(AMSTRAD_PPI_A);
    outb(portb, AMSTRAD_PPI_B);
    restore_flags(flags);
    return status;
}

static unsigned char INITPROC AmstradOptRead(unsigned int probe_port,
                                             unsigned char *printer_ctrl)
{
    unsigned int flags;
    unsigned char ctrl;

    save_flags(flags);
    clr_irq();
    (void)inb(probe_port);
    ctrl = inb(AMSTRAD_PRINTER_CTRL);
    restore_flags(flags);
    if (printer_ctrl)
        *printer_ctrl = ctrl;
    return (ctrl & AMSTRAD_PC1640_OPT) ? 1 : 0;
}

static void INITPROC AmstradIgaReadSwitches(void)
{
    unsigned char ctrl;

    amstrad_iga_active = 0;
    amstrad_iga_status1 = AmstradStatus1();
    amstrad_iga_ddm = (amstrad_iga_status1 & AMSTRAD_STATUS1_DDM) >>
                      AMSTRAD_STATUS1_DDM_SH;
    if ((amstrad_iga_status1 & AMSTRAD_STATUS1_MASK) != AMSTRAD_STATUS1_FIXED)
        goto apply_override;
    if (AmstradOptRead(AMSTRAD_CGA_INDEX, &ctrl))
        goto apply_override;

    amstrad_iga_sw6 = (ctrl & AMSTRAD_SW6) ? 1 : 0;
    amstrad_iga_sw7 = (ctrl & AMSTRAD_SW7) ? 1 : 0;
    amstrad_iga_sw9 = AmstradOptRead(AMSTRAD_SW9_PROBE, NULL);
    amstrad_iga_sw10 = AmstradOptRead(AMSTRAD_SW10_PROBE, NULL);

    /*
     * Measured on a real PC1640: with the internal adaptor active (SW10
     * "off"), the SW10 option read returns 0; SW10 "on" disables the
     * adaptor and reads 1.  86Box models this line inverted (always 1 with
     * its internal EGA running), so under that emulator use iga=1 to force
     * the feature on for testing.
     */
    if (!amstrad_iga_sw10)
        amstrad_iga_active = 1;

apply_override:
    /* /bootopts iga= diagnostic override: 1 forces on, 2 forces off. */
    if (amstrad_iga_override == 1)
        amstrad_iga_active = 1;
    else if (amstrad_iga_override == 2)
        amstrad_iga_active = 0;
}

static void INITPROC AmstradIgaPrintSwitches(void)
{
    printk("Amstrad IGA switch state: %s st1=0x%x ddm=%u sw6=%u sw7=%u sw9=%u sw10=%u%s\n",
           amstrad_iga_active ? "enabled" : "disabled",
           amstrad_iga_status1, amstrad_iga_ddm, amstrad_iga_sw6,
           amstrad_iga_sw7, amstrad_iga_sw9, amstrad_iga_sw10,
           amstrad_iga_override ? " (forced)" :
           (amstrad_iga_sw10 ? " internal-adapter-off" : ""));
}

/*
 * Map a screen cell to its bank-absolute video word, wrapping inside this
 * VC's scroll ring.  Callers guarantee page_words is nonzero and the shift
 * plus cell never reaches twice the ring size.
 */
static unsigned int VideoCell(Console * C, unsigned int cell)
{
    cell += C->crtc_offset - C->crtc_home;
    if (cell >= C->page_words)
        cell -= C->page_words;
    return C->crtc_home + cell;
}

static void BiosSetDisplayStart(Console * C)
{
    if (!amstrad_iga_active || !C->page_words)
        return;
    pokew(0x4e, BIOSSEG, C->crtc_offset << 1);
    pokeb(0x62, BIOSSEG, (unsigned char)(C->crtc_home / C->page_words));
    pokew(0x63, BIOSSEG, C->crtc_base);
}

static void BiosSetCursor(Console * C)
{
    unsigned int page;

    if (!amstrad_iga_active || !C->page_words || C != Visible[C->display])
        return;
    page = C->crtc_home / C->page_words;
    pokew(0x50 + (page << 1), BIOSSEG,
          ((unsigned int)C->cy << 8) | (unsigned char)C->cx);
}

static void SetDisplayPage(Console * C)
{
    outw((C->crtc_offset & 0xff00) | 0x0c, C->crtc_base);
    outw((C->crtc_offset << 8) | 0x0d, C->crtc_base);
    BiosSetDisplayStart(C);
}

static void PositionCursor(Console * C)
{
    unsigned int Pos = C->cx + C->Width * C->cy + C->crtc_offset;

    if (C->page_words)
        Pos = VideoCell(C, C->cx + C->Width * C->cy);
    outb(14, C->crtc_base);
    outb(Pos >> 8, C->crtc_base + 1);
    outb(15, C->crtc_base);
    outb(Pos, C->crtc_base + 1);
    BiosSetCursor(C);
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
    if (amstrad_iga_active && C == Visible[C->display])
        pokew(0x60, BIOSSEG, v);
}

static void VideoWrite(Console * C, int c)
{
    if (C->page_words) {
        pokew(VideoCell(C, C->cx + C->cy * C->Width) << 1, C->vseg,
              (C->attr << 8) | (c & 255));
        return;
    }
    pokew(((C->cx + C->cy * C->Width) << 1) + C->vseg_offset, C->vseg,
          (C->attr << 8) | (c & 255));
}

static void ClearRange(Console * C, int x, int y, int x2, int y2)
{
    int vp;

    if (C->page_words) {
        unsigned int cell = x + y * C->Width;

        x2 = x2 - x + 1;
        do {
            for (x = 0; x < x2; x++)
                pokew(VideoCell(C, cell + x) << 1, C->vseg,
                      (C->attr << 8) | ' ');
            cell += C->Width;
        } while (++y <= y2);
        return;
    }
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

static void VideoCopyLine(Console * C, int dstrow, int srcrow)
{
    int x;

    for (x = 0; x < C->Width; x++)
        pokew(VideoCell(C, x + dstrow * C->Width) << 1, C->vseg,
              peekw(VideoCell(C, x + srcrow * C->Width) << 1, C->vseg));
}

static int CanHwScroll(Console * C, int y)
{
    return amstrad_iga_active && y == 0 && C->type != OT_MDA &&
           C->page_words >= (unsigned int)(C->Width * (C->Height + 1));
}

static void HwScrollUp(Console * C)
{
    unsigned int shift = C->crtc_offset - C->crtc_home + C->Width;

    if (shift >= C->page_words)
        shift -= C->page_words;
    C->crtc_offset = C->crtc_home + shift;
    C->vseg_offset = C->crtc_offset << 1;
    if (C == Visible[C->display])
        SetDisplayPage(C);
    ClearRange(C, 0, C->Height - 1, C->Width - 1, C->Height - 1);
}

static void ScrollUp(Console * C, int y)
{
    int vp;
    int MaxRow = C->Height - 1;
    int MaxCol = C->Width - 1;

    if (C->page_words) {
        if (CanHwScroll(C, y)) {
            HwScrollUp(C);
            return;
        }
        if (C->crtc_offset != C->crtc_home) {
            while (y < MaxRow) {
                VideoCopyLine(C, y, y + 1);
                y++;
            }
            ClearRange(C, 0, MaxRow, MaxCol, MaxRow);
            return;
        }
    }
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

    if (C->page_words && C->crtc_offset != C->crtc_home) {
        while (yy > y) {
            VideoCopyLine(C, yy, yy - 1);
            yy--;
        }
        ClearRange(C, 0, y, C->Width - 1, y);
        return;
    }
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

/*
 * The Amstrad BIOS itself scrolls by moving the CRTC start address, so the
 * ring may hold stale rows anywhere in the bank at boot.  Start each VC from
 * a cleared ring and a homed cursor, as the hardware-tested driver did.
 */
static void INITPROC AmstradIgaClearInit(Console * C)
{
    unsigned int w;

    if (!C->page_words)
        return;
    for (w = 0; w < C->page_words; w++)
        pokew((C->crtc_home + w) << 1, C->vseg, (A_DEFAULT << 8) | ' ');
    C->cx = C->cy = 0;
    C->XN = 0;
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
    AmstradIgaReadSwitches();

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
        C->crtc_home = C->crtc_offset;
        C->page_words = (amstrad_iga_active && !UseRambuf &&
            PageSizeW >= (unsigned int)(Width * Height)) ? PageSizeW : 0;
        AmstradIgaClearInit(C);
        if (i == 0 && C->page_words) {
            SetDisplayPage(C);
            PositionCursor(C);
        }

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
    AmstradIgaPrintSwitches();
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
