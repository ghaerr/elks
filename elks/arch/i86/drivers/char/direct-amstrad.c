/*
 * Direct console video output for Amstrad PC1512/PC1640 integrated Paradise PEGA (IGA).
 *
 * The PEGA answers at the CGA/MDA addresses but adds a hardware scroll
 * that must be driven through the 6845 start address, and reports the
 * board's SW10 setting through an extra status bit.  Kept as a separate
 * driver rather than as conditionals in console-direct.c.
 *
 * Amstrad PC1512/PC1640 integrated graphics adapter (Paradise PEGA "IGA").
 * The PPI status-1 byte carries a fixed Amstrad fingerprint plus the display
 * default mode switches; the PC1640 option register, read through the
 * printer control port after strobing an option address, exposes SW6-SW10.
 * SW10 set means the internal adapter is disabled by an external card.
 * When active, the driver keeps the BIOS data area coherent and scrolls by
 * moving the CRTC start address through the video bank instead of copying.
 */
#include <arch/irq.h>

#ifdef CONFIG_CONSOLE_DUAL
#error CONFIG_CONSOLE_AMSTRAD_IGA does not support CONFIG_CONSOLE_DUAL
#endif

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
extern int iga_opts;        /* /bootopts iga= 1 forces on, 2 forces off */

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
    if (iga_opts == 1)
        amstrad_iga_active = 1;
    else if (iga_opts == 2)
        amstrad_iga_active = 0;
}

static void INITPROC AmstradIgaPrintSwitches(void)
{
    printk("Amstrad IGA switch state: %s st1=0x%x ddm=%u sw6=%u sw7=%u sw9=%u sw10=%u%s\n",
           amstrad_iga_active ? "enabled" : "disabled",
           amstrad_iga_status1, amstrad_iga_ddm, amstrad_iga_sw6,
           amstrad_iga_sw7, amstrad_iga_sw9, amstrad_iga_sw10,
           iga_opts ? " (forced)" :
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
