/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * CGA Screen Driver
 * This driver is created and modifed, based on EGA/VGA driver.
 * T. Yamada 2024
 *
 * 	For CGA, 2 color, 640x200 resolution
 * 	This driver uses bios for graphical setting.
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

#define _MK_FP(seg,off) ((void __far *)((((unsigned long)(seg)) << 16) | (off)))

/* CGA driver entry points*/
static int  CGA_open(PSD psd);
static void CGA_close(PSD psd);
static void CGA_getscreeninfo(PSD psd,PSCREENINFO psi);;
static void CGA_setpalette(PSD psd,int first,int count,RGBENTRY *pal);
static void CGA_drawpixel(PSD psd,COORD x, COORD y, PIXELVAL c);
static PIXELVAL CGA_readpixel(PSD psd,COORD x, COORD y);
static void CGA_drawhline(PSD psd,COORD x1, COORD x2, COORD y, PIXELVAL c);
static void CGA_drawvline(PSD psd,COORD x,COORD y1,COORD y2,PIXELVAL c);
static void CGA_fillrect(PSD psd,COORD x1,COORD y1,COORD x2,COORD y2,PIXELVAL c);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	CGA_open,
	CGA_close,
	CGA_getscreeninfo,
	CGA_setpalette,
	CGA_drawpixel,
	CGA_readpixel,
	CGA_drawhline,
	CGA_drawvline,
	CGA_fillrect,
	pcrom_getfontinfo,
	pcrom_gettextsize,
	pcrom_gettextbits,
	ega_blit
};

/* operating mode*/
static BOOL MONOMODE = FALSE;	/* monochrome mode*/

/* int10 functions*/
#define FNGR640x200	0x0006	/* function for graphics mode 640x200x2*/
#define FNTEXT		0x0003	/* function for 80x25 text mode*/

static int
CGA_open(PSD psd)
{
	/* setup operating mode from environment variable*/
	if(getenv("MONOMODE"))
		MONOMODE = TRUE;
	else MONOMODE = FALSE;

	int10(FNGR640x200, 0);

	/* init driver variables depending on cga mode*/
	psd->xres = 640;
	psd->yres = 200;
	psd->planes = 1;

	if (MONOMODE) {
		psd->bpp = 1;
		psd->ncolors = 2;
	} else {
		// To run colored applications
		psd->bpp = 4;
		psd->ncolors = 16;
	}

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

	/* FIXME: add palette code*/
	return 1;
}

static void
CGA_close(PSD psd)
{
	/* init bios 80x25 text mode*/
	int10(FNTEXT, 0);
}

static void
CGA_getscreeninfo(PSD psd,PSCREENINFO psi)
{
	psi->rows = psd->yres;
	psi->cols = psd->xres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = NUMBER_FONTS;

	/* 640x200 */
	psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
	psi->ydpcm = 11;	/* assumes screen height of 18 cm*/
}

static void
CGA_setpalette(PSD psd,int first,int count,RGBENTRY *pal)
{
	/* not yet implemented, std 16 color palette assumed*/
}

static void
CGA_drawpixel(PSD psd,COORD x, COORD y, PIXELVAL c)
{
#if HAVEBLIT
	if(psd->flags & PSF_MEMORY)
		mempl4_drawpixel(psd, x, y, c);
	else 
#endif
		ega_drawpixel(psd, x, y, c);
}

static PIXELVAL
CGA_readpixel(PSD psd,COORD x, COORD y)
{
#if HAVEBLIT
	if(psd->flags & PSF_MEMORY)
		return mempl4_readpixel(psd, x, y);
#endif
	return ega_readpixel(psd, x, y);
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
CGA_drawhline(PSD psd,COORD x1, COORD x2, COORD y, PIXELVAL c)
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
CGA_drawvline(PSD psd,COORD x, COORD y1, COORD y2, PIXELVAL c)
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
CGA_fillrect(PSD psd,COORD x1, COORD y1, COORD x2, COORD y2, PIXELVAL c)
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
