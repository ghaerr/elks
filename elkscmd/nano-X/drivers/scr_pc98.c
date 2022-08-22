/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * PC-98 16 color 4 planes Screen Driver
 * This driver is created and modifed, based on EGA/VGA driver.
 * T. Yamada 2022
 *
 * 	For PC-98, 640x400 resolution
 * 	This driver uses LIO for graphical setting.
 *
 * 	This file itself doesn't know about any planar or packed arrangement, relying soley
 * 	on the following external routines for all graphics drawing:
 * 		ega_init, ega_drawpixel, ega_readpixel,
 * 		ega_drawhorzline, ega_drawvertline
 */

#include <linuxmt/ntty.h>
#include <stdio.h>
#include <stdlib.h>
#include "../device.h"
#include "vgaplan4.h"
#include "romfont.h"

#define static

#define LIOSEG 0xF990 /* ROM Segment for LIO */
#define LIOINT 0xA0   /* Starting LIO interrupt number */

#define LIO_M_SIZE 5200

#define _MK_FP(seg,off) ((void __far *)((((unsigned long)(seg)) << 16) | (off)))

/* PC98 driver entry points*/
static int  PC98_open(PSD psd);
static void PC98_close(PSD psd);
static void PC98_getscreeninfo(PSD psd,PSCREENINFO psi);;
static void PC98_setpalette(PSD psd,int first,int count,RGBENTRY *pal);
static void PC98_drawpixel(PSD psd,COORD x, COORD y, PIXELVAL c);
static PIXELVAL PC98_readpixel(PSD psd,COORD x, COORD y);
static void PC98_drawhline(PSD psd,COORD x1, COORD x2, COORD y, PIXELVAL c);
static void PC98_drawvline(PSD psd,COORD x,COORD y1,COORD y2,PIXELVAL c);
static void PC98_fillrect(PSD psd,COORD x1,COORD y1,COORD x2,COORD y2,PIXELVAL c);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	PC98_open,
	PC98_close,
	PC98_getscreeninfo,
	PC98_setpalette,
	PC98_drawpixel,
	PC98_readpixel,
	PC98_drawhline,
	PC98_drawvline,
	PC98_fillrect,
	pcrom_getfontinfo,
	pcrom_gettextsize,
	pcrom_gettextbits,
	ega_blit
};

void int_A0(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "int $0xA0;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void int_A1(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "mov $0x0000,%%bx;"
                      "int $0xA1;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void int_A2(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "mov $0x0000,%%bx;"
                      "int $0xA2;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void int_A3(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "mov $0x0000,%%bx;"
                      "int $0xA3;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

static int
PC98_open(PSD psd)
{
	unsigned char *lio_malloc;
	unsigned char __far *lio_m;
	unsigned int lio_m_seg;

	unsigned long __far *intvec;
	unsigned int __far *lioaddr;
	unsigned int i;

	intvec = (unsigned long __far *) _MK_FP(0, LIOINT<<2); /* interrupt vector for INT 0xA0 */
	lioaddr = (unsigned int __far *) _MK_FP(LIOSEG, 6);   /* Starting Rom Address for INT 0xA0 handler */

	// Set interrupt vector 0xA0 - 0xAF
	for (i = 0; i < 16; i++) {
		*intvec = (unsigned long) _MK_FP(LIOSEG, *lioaddr);
		intvec++;
		lioaddr += 2;
	}

	// Allocate memory for LIO
	lio_malloc = malloc(LIO_M_SIZE);
	lio_m = (unsigned char __far *) lio_malloc;

	lio_m_seg = (unsigned int) ((((unsigned long) lio_m) >> 16) + ((((unsigned long) lio_m) & 0xFFFF) >> 4) + 1);

	lio_m = (unsigned char __far *) (((unsigned long) lio_m_seg) << 16);

	int_A0(lio_m_seg); // Init

	lio_m[0] = 0x03; // Color 640x400
	lio_m[1] = 0x00;
	lio_m[2] = 0x00;
	lio_m[3] = 0x01;
	int_A1(lio_m_seg);

	lio_m[0] = 0x00;
	lio_m[1] = 0x00;
	lio_m[2] = 0x00;
	lio_m[3] = 0x00;
	lio_m[4] = 0x7F; // 639
	lio_m[5] = 0x02;
	lio_m[6] = 0x8F; // 399
	lio_m[7] = 0x01;
	lio_m[8] = 0xFF;
	lio_m[9] = 0xFF;
	int_A2(lio_m_seg);

	lio_m[0] = 0xFF;
	lio_m[1] = 0xFF;
	lio_m[2] = 0xFF;
	lio_m[3] = 0x02; // 16 Color mode
	int_A3(lio_m_seg);

	free(lio_malloc);

	/* init driver variables for PC-98*/
	psd->xres = 640;
	psd->yres = 400;
	psd->planes = 4;
	psd->bpp = 4;
	psd->ncolors = 16;
	psd->pixtype = PF_PALETTE;
#if HAVEBLIT
	psd->flags = PSF_SCREEN | PSF_HAVEBLIT;
#else
	psd->flags = PSF_SCREEN;
#endif

	/* init planes driver (sets psd->addr and psd->linelen)*/
	ega_init(psd);

	/* init pc rom font routines*/
	pcrom_init(psd);
	outb(0x0B,0x68); // Dot Access for font
	/* FIXME: add palette code*/
	return 1;
}

static void
PC98_close(PSD psd)
{
	outb(0x0C,0xA2);   // GDC Stop
	outb(0x0A,0x68);   // Code Access for font
}

static void
PC98_getscreeninfo(PSD psd,PSCREENINFO psi)
{
	psi->rows = psd->yres;
	psi->cols = psd->xres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = NUMBER_FONTS;

	/* 640x400 */
	psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
	psi->ydpcm = 27;	/* assumes screen height of 15 cm*/
}

static void
PC98_setpalette(PSD psd,int first,int count,RGBENTRY *pal)
{
	/* not yet implemented, std 16 color palette assumed*/
}

static void
PC98_drawpixel(PSD psd,COORD x, COORD y, PIXELVAL c)
{
#if HAVEBLIT
	if(psd->flags & PSF_MEMORY)
		mempl4_drawpixel(psd, x, y, c);
	else 
#endif
		ega_drawpixel(psd, x, y, c);
}

static PIXELVAL
PC98_readpixel(PSD psd,COORD x, COORD y)
{
#if HAVEBLIT
	if(psd->flags & PSF_MEMORY)
		return mempl4_readpixel(psd, x, y);
#endif
	return ega_readpixel(psd, x, y);
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
PC98_drawhline(PSD psd,COORD x1, COORD x2, COORD y, PIXELVAL c)
{
	++x2;		/* draw final point*/
#if HAVEBLIT
	if(psd->flags & PSF_MEMORY)
		mempl4_drawhorzline(psd, x1, x2, y, c);
	else 
#endif
		ega_drawhorzline(psd, x1, x2, y, c);
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
PC98_drawvline(PSD psd,COORD x, COORD y1, COORD y2, PIXELVAL c)
{
	++y2;		/* draw final point*/
#if HAVEBLIT
	if(psd->flags & PSF_MEMORY)
		mempl4_drawvertline(psd, x, y1, y2, c);
	else 
#endif
		ega_drawvertline(psd, x, y1, y2, c);
}

static void
PC98_fillrect(PSD psd,COORD x1, COORD y1, COORD x2, COORD y2, PIXELVAL c)
{
	++x2;		/* draw last point*/
#if HAVEBLIT
	if(psd->flags & PSF_MEMORY) {
		while(y1 <= y2)
			mempl4_drawhorzline(psd, x1, x2, y1++, c);
	} else 
#endif
		while(y1 <= y2)
			ega_drawhorzline(psd, x1, x2, y1++, c);
}
