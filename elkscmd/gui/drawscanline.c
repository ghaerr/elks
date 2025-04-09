/* vga_drawscanline - draw planar 4 VGA from 16 color byte array   */
/* 5 Apr 2025 Greg Haerr ported from SVGALib for ELKS              */

/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

/* 21 January 1995 - added vga_readscanline(), added support for   */
/* non 8-pixel aligned scanlines in 16 color mode. billr@rastergr.com */

#include <string.h>
#include "graphics.h"
#include "vgalib.h"

/* used to decompose color value into bits (for fast scanline drawing) */
union bits {
    struct {
        unsigned char bit3;
        unsigned char bit2;
        unsigned char bit1;
        unsigned char bit0;
    } b;
    unsigned long i;
};

/* color decompositions */
static union bits color16[16] =
{
    {{0, 0, 0, 0}},
    {{0, 0, 0, 1}},
    {{0, 0, 1, 0}},
    {{0, 0, 1, 1}},
    {{0, 1, 0, 0}},
    {{0, 1, 0, 1}},
    {{0, 1, 1, 0}},
    {{0, 1, 1, 1}},
    {{1, 0, 0, 0}},
    {{1, 0, 0, 1}},
    {{1, 0, 1, 0}},
    {{1, 0, 1, 1}},
    {{1, 1, 0, 0}},
    {{1, 1, 0, 1}},
    {{1, 1, 1, 0}},
    {{1, 1, 1, 1}}
};

/* mask for end points in plane buffer mode */
static unsigned char mask[8] =
{
    0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01
};

/* display plane buffers (for fast scanline drawing) */
/* 80 bytes -> max 640 pixel per line (2/16 colors) */
static unsigned char plane0[80];
static unsigned char plane1[80];
static unsigned char plane2[80];
static unsigned char plane3[80];

#if     defined(__ia16__) || defined(__C86__)
#define MEMCPY(dstoff, src, n)  fdstmemcpy(dstoff, EGA_BASE, src, n)
#elif   defined(__WATCOMC__)
#define MEMCPY(dstoff, src, n)  fmemcpy(MK_FP(EGA_BASE, dstoff), src, n);
#else   /* future compiler */
static void MEMCPY(unsigned int dstoff, unsigned char *src, int n)
{
    unsigned char __far *dst = MK_FP(EGA_BASE, dstoff);
    while (n--)
        *dst++ = *src++;
}
#endif

void vga_drawscanline(unsigned char *colors, int x, int y, int length)
{
    int i, j, k, first, last, l1;
    unsigned int offset, eoffs, soffs, ioffs;
    unsigned int soffmask, eoffmask;
    union bits bytes;
    
    if (length > 640)
        length = 640;
    k = 0;
    soffs = ioffs = (x & 0x7);  /* starting offset into first byte */
    eoffs = (x + length) & 0x7;     /* ending offset into last byte */
    for (i = 0; i < length;) {
        bytes.i = 0;
        first = i;
        last = i + 8 - ioffs;
        if (last > length)
            last = length;
        for (j = first; j < last; j++, i++) {
#ifdef __C86__
            bytes.i = (bytes.i << 1);
            bytes.i |= color16[colors[j]].i;
#else
            bytes.i = (bytes.i << 1) | color16[colors[j]&15].i;
#endif
        }
        plane0[k] = bytes.b.bit0;
        plane1[k] = bytes.b.bit1;
        plane2[k] = bytes.b.bit2;
        plane3[k++] = bytes.b.bit3;
        ioffs = 0;
    }
    if (soffs)
        soffmask = ~mask[soffs];
    if (eoffs) {
        /* fixup last byte */
        k--;
        bytes.i <<= (8 - eoffs);
        plane0[k] = bytes.b.bit0;
        plane1[k] = bytes.b.bit1;
        plane2[k] = bytes.b.bit2;
        plane3[k++] = bytes.b.bit3;
        eoffmask = mask[eoffs];
    }
    offset = (y << 6) + (y << 4) + (x >> 3);    /* y * 640/8 + x / 8 */
    /* k currently contains number of bytes to write */
    l1 = k;
    /* make k the index of the last byte to write */
    k--;

    set_enable_sr(0x00);        /* REG 1: disable Set/Reset Register */
    set_mask(0xff);             /* REG 8: write to all bits */

    set_write_planes(0x01);     /* REG 2: select write map mask register */
    set_read_plane(0x00);       /* REG 4: select read map mask register */
    if (soffs)
        plane0[0] |= asm_getbyte(offset) & soffmask;
    if (eoffs)
        plane0[k] |= asm_getbyte(offset + k) & eoffmask;
    MEMCPY(offset, plane0, l1);
    
    set_write_planes(0x02);     /* REG 2: write plane 1 */
    set_read_plane(0x01);       /* REG 4: read plane 1 */
    if (soffs)
        plane1[0] |= asm_getbyte(offset) & soffmask;
    if (eoffs)
        plane1[k] |= asm_getbyte(offset + k) & eoffmask;
    MEMCPY(offset, plane1, l1);
    
    set_write_planes(0x04);     /* REG 2: write plane 2 */
    set_read_plane(0x02);       /* REG 4: read plane 2 */
    if (soffs)
        plane2[0] |= asm_getbyte(offset) & soffmask;
    if (eoffs)
        plane2[k] |= asm_getbyte(offset + k) & eoffmask;
    MEMCPY(offset, plane2, l1);
    
    set_write_planes(0x08);     /* REG 2: write plane 3 */
    set_read_plane(0x03);       /* REG 4: read plane 3 */
    if (soffs)
        plane3[0] |= asm_getbyte(offset) & soffmask;
    if (eoffs)
        plane3[k] |= asm_getbyte(offset + k) & eoffmask;
    MEMCPY(offset, plane3, l1);
    
    set_write_planes(0x0f);     /* REG 2: restore map mask register */
    set_enable_sr(0xff);        /* REG 1: enable Set/Reset Register */

#if UNUSED  /* 1bpp routine */
    set_enable_sr(0x00);        /* REG 1: disable Set/Reset Register */
    set_mask(0xff);             /* REG 8: write to all bits */
    set_write_planes(0x0f);     /* REG 2: write to all planes */

    MEMCPY((y << 6) + (y << 4) + (x >> 3), colors, length);  /* y * 80 + x / 8 */
    
    set_write_planes(0x0f);     /* REG 2: restore map mask register */
    set_enable_sr(0xff);        /* REG 1: enable Set/Reset Register */
#endif
}
