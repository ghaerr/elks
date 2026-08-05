/*
 * direct console video output for IBM PC.
 */

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
