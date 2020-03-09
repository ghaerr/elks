/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * EGA/VGA 16 color 4 planes Screen Driver, PC bios version
 * 	This driver uses int10 bios to set/reset graphics/text modes,
 * 	and to get the address of the ROM character font which
 * 	is used for the character bitmaps.  All other access to the
 * 	hardware is controlled through this driver.
 *
 * 	Blitting enabled with #define HAVEBLIT in vgaplan4.h
 *
 * 	This driver links with one of two other files, vgaplan4.c,
 * 	the portable VGA 4 planes 16 color driver, or asmplan4.s, which
 * 	is 8086 assembly language for speed.  This file itself
 * 	doesn't know about any planar or packed arrangement, relying soley
 * 	on the following external routines for all graphics drawing:
 * 		ega_init, ega_drawpixel, ega_readpixel,
 * 		ega_drawhorzline, ega_drawvertline
 * 	In addition, romfont.c is linked in for the PC rom font routines.
 *
 * 	All text/font drawing code is based on the above routines and
 * 	the included entry points for getting the ROM bitmap data.  Compiled
 * 	in fonts aren't supported for size reasons.  scr_fb supports them.
 *
 * 	If the environment variable EGAMODE is set, the driver implements
 *	the EGA 640x350 (mode 10h) resolution, otherwise 640x480 (mode 12h)
 *	graphics mode is set.
 *
 *	The environment variable CHARHEIGHT if set will set the assumed rom
 *	font character height, which defaults to 14.
 *
 * 	If #define HWINIT 1 is set, the file vgainit.c is
 * 	used to provide non-bios direct hw initialization of the VGA
 * 	chipset.
 */
#define HWINIT	0		/* =1 for non-bios direct hardware init*/

#if ELKS
#include <linuxmt/ntty.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "../device.h"
#include "vgaplan4.h"
#include "romfont.h"

#define static

/* VGA driver entry points*/
static int  VGA_open(PSD psd);
static void VGA_close(PSD psd);
static void VGA_getscreeninfo(PSD psd,PSCREENINFO psi);;
static void VGA_setpalette(PSD psd,int first,int count,RGBENTRY *pal);
static void VGA_drawpixel(PSD psd,COORD x, COORD y, PIXELVAL c);
static PIXELVAL VGA_readpixel(PSD psd,COORD x, COORD y);
static void VGA_drawhline(PSD psd,COORD x1, COORD x2, COORD y, PIXELVAL c);
static void VGA_drawvline(PSD psd,COORD x,COORD y1,COORD y2,PIXELVAL c);
static void VGA_fillrect(PSD psd,COORD x1,COORD y1,COORD x2,COORD y2,PIXELVAL c);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	VGA_open,
	VGA_close,
	VGA_getscreeninfo,
	VGA_setpalette,
	VGA_drawpixel,
	VGA_readpixel,
	VGA_drawhline,
	VGA_drawvline,
	VGA_fillrect,
	pcrom_getfontinfo,
	pcrom_gettextsize,
	pcrom_gettextbits,
	ega_blit
};

/* operating mode*/
static BOOL VGAMODE = TRUE;	/* ega or vga screen rows*/

/* int10 functions*/
#define FNGR640x480	0x0012	/* function for graphics mode 640x480x16*/
#define FNGR640x350	0x0010	/* function for graphics mode 640x350x16*/
#define FNTEXT		0x0003	/* function for 80x25 text mode*/

static int
VGA_open(PSD psd)
{
	/* setup operating mode from environment variable*/
	if(getenv("EGAMODE"))
		VGAMODE = FALSE;
	else VGAMODE = TRUE;

#if ELKS
	/* disallow console switching while in graphics mode*/
	//ioctl(0, DCGET_GRAPH); /*continue if failed*/
#endif

#if HWINIT
	/* enter graphics mode*/
	ega_hwinit(psd);
#else
	/* init bios graphics mode*/
	int10(VGAMODE? FNGR640x480: FNGR640x350, 0);
#endif

	/* init driver variables depending on ega/vga mode*/
	psd->xres = 640;
	psd->yres = VGAMODE? 480: 350;
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
#if 0
	ROM_CHAR_HEIGHT = VGAMODE? 16: 14;
#endif
	/* FIXME: add palette code*/
	return 1;
}

static void
VGA_close(PSD psd)
{
#if ELKS
	/* allow console switching again*/
	//ioctl(0, DCREL_GRAPH);
#endif
#if HWINIT
	ega_hwterm(psd);
#else
	/* init bios 80x25 text mode*/
	int10(FNTEXT, 0);
#endif
}

static void
VGA_getscreeninfo(PSD psd,PSCREENINFO psi)
{
	psi->rows = psd->yres;
	psi->cols = psd->xres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = NUMBER_FONTS;

	if(VGAMODE) {
		/* VGA 640x480*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
	} else {
		/* EGA 640x350*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
	}

#if ETA4000
	/* SVGA 800x600*/
	psi->xdpcm = 33;		/* assumes screen width of 24 cm*/
	psi->ydpcm = 33;		/* assumes screen height of 18 cm*/
#endif
}

static void
VGA_setpalette(PSD psd,int first,int count,RGBENTRY *pal)
{
	/* not yet implemented, std 16 color palette assumed*/
}

static void
VGA_drawpixel(PSD psd,COORD x, COORD y, PIXELVAL c)
{
#if HAVEBLIT
	if(psd->flags & PSF_MEMORY)
		mempl4_drawpixel(psd, x, y, c);
	else
#endif
		ega_drawpixel(psd, x, y, c);
}

static PIXELVAL
VGA_readpixel(PSD psd,COORD x, COORD y)
{
#if HAVEBLIT
	if(psd->flags & PSF_MEMORY)
		return mempl4_readpixel(psd, x, y);
#endif
	return ega_readpixel(psd, x, y);
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
VGA_drawhline(PSD psd,COORD x1, COORD x2, COORD y, PIXELVAL c)
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
VGA_drawvline(PSD psd,COORD x, COORD y1, COORD y2, PIXELVAL c)
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
VGA_fillrect(PSD psd,COORD x1, COORD y1, COORD x2, COORD y2, PIXELVAL c)
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
