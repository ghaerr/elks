/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver using SVGA Library
 *
 * This driver requires the following SVGA entry points:
 * 	vga_init, vga_setmode,
 * 	vga_drawpixel, vga_getpixel,
 * 	vga_setegacolor, vga_drawline,
 *	vga_getscansegment, vga_drawscansegment
 *
 * All graphics drawing primitives are based on top of these functions.
 *
 * This file also contains the generalized low-level font/text
 * drawing routines, which will be split out into another file.
 * Both fixed and proportional fonts are supported.
 */
#include <stdio.h>
#include "../device.h"
#include "genfont.h"
#include <vga.h>

/* specific vgalib driver entry points*/
static int  SVGA_open(PSD psd);
static void SVGA_close(PSD psd);
static void SVGA_getscreeninfo(PSD psd,PSCREENINFO psi);
static void SVGA_setpalette(PSD psd,int first,int count,RGBENTRY *pal);
static void SVGA_drawpixel(PSD psd,COORD x, COORD y, PIXELVAL c);
static PIXELVAL SVGA_readpixel(PSD psd,COORD x, COORD y);
static void SVGA_drawhline(PSD psd,COORD x1, COORD x2, COORD y, PIXELVAL c);
static void SVGA_drawvline(PSD psd,COORD x, COORD y1, COORD y2, PIXELVAL c);
static void SVGA_fillrect(PSD psd,COORD x1,COORD y1,COORD x2,COORD y2,PIXELVAL c);
static void SVGA_blit(PSD dstpsd,COORD destx,COORD desty,COORD w,COORD h,
		PSD srcpsd,COORD srcx,COORD srcy,int op);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	SVGA_open,
	SVGA_close,
	SVGA_getscreeninfo,
	SVGA_setpalette,
	SVGA_drawpixel,
	SVGA_readpixel,
	SVGA_drawhline,
	SVGA_drawvline,
	SVGA_fillrect,
	gen_getfontinfo,
	gen_gettextsize,
	gen_gettextbits,
	SVGA_blit
};

extern MODE gr_mode;	/* temp kluge*/

static int
SVGA_open(PSD psd)
{
	int		mode;
	vga_modeinfo *	modeinfo;

	vga_init();
	//mode = G640x480x256;
	mode = G640x480x16;
	vga_setmode(mode);
	modeinfo = vga_getmodeinfo(mode);

	psd->xres = modeinfo->width;
	psd->yres = modeinfo->height;
	psd->linelen = modeinfo->linewidth;
	psd->planes = 1;
	psd->bpp = modeinfo->bytesperpixel;	// FIXME??
	psd->ncolors = modeinfo->colors;
	psd->flags = PSF_SCREEN;
	psd->addr = 0;		// FIXME

	/* note: must change psd->pixtype here for truecolor systems*/
	psd->pixtype = PF_PALETTE;
	return 1;
}

static void
SVGA_close(PSD psd)
{
	vga_setmode(TEXT);
}

static void
SVGA_getscreeninfo(PSD psd,PSCREENINFO psi)
{
	psi->rows = psd->yres;
	psi->cols = psd->xres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = NUMBER_FONTS;

	if(scrdev.yres > 480) {
		/* SVGA 800x600*/
		psi->xdpcm = 33;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 33;	/* assumes screen height of 18 cm*/
	} else if(scrdev.yres > 350) {
		/* VGA 640x480*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
	} else {
		/* EGA 640x350*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
	}
}

static void
SVGA_setpalette(PSD psd,int first,int count,RGBENTRY *pal)
{
	while(first < 256 && count-- > 0) {
		vga_setpalette(first++, pal->r>>2, pal->g>>2, pal->b>>2);
		++pal;
	}
}

static void
SVGA_drawpixel(PSD psd,COORD x, COORD y, PIXELVAL c)
{
	unsigned char gline, line;

	if(gr_mode == MODE_SET) {
		vga_setegacolor(c);
		vga_drawpixel(x, y);
		return;
	}
	/*
	 * This fishery is required because vgalib doesn't support
	 * xor drawing mode without acceleration.
	 */
	vga_getscansegment(&gline, x, y, 1);
	line = c;
	line ^= gline;
	vga_drawscansegment(&line, x, y, 1);
}

static PIXELVAL
SVGA_readpixel(PSD psd,COORD x, COORD y)
{
	return vga_getpixel(x, y);
}

static void
SVGA_drawhline(PSD psd,COORD x1, COORD x2, COORD y, PIXELVAL c)
{
	int 	i, width;
	unsigned char getline[640];
	static int lastcolor = -1;
	static int lastwidth = -1;
	static unsigned char line[640];

	/*
	 * All this fishery is required for two reasons:
	 * one, vga_drawline is way too slow to be called to fill
	 * rectangles, so vga_drawscansegment is used instead.  In
	 * addition, vgalib doesn't support xor drawing mode
	 * without acceleration!!, so we've got to do it ourselves
	 * with vga_getscansegment...
	 */
	width = x2-x1+1;

	/* this is faster than calling vga_drawline !!!*/
	if(width != lastwidth || c != lastcolor) {
		lastwidth = width;
		lastcolor = c;
		for(i=0; i<width; ++i)
			line[i] = c;
	}
	if(gr_mode == MODE_XOR) {
		vga_getscansegment(getline, x1, y, width);
		for(i=0; i<width; ++i)
			line[i] ^= getline[i];
		lastwidth = -1;
	}
	vga_drawscansegment(line, x1, y, width);

	/*
	 * Non-fishery version is *slow* and doesn't support XOR.
	vga_setegacolor(c);
	vga_drawline(x1, y, x2, y2);
	 */
}

static void
SVGA_drawvline(PSD psd,COORD x, COORD y1, COORD y2, PIXELVAL c)
{
	if(gr_mode == MODE_SET) {
		vga_setegacolor(c);
		vga_drawline(x, y1, x, y2);
	}

	/* slower version required for XOR drawing support*/
	while(y1 <= y2)
		SVGA_drawpixel(psd, x, y1++, c);
}

static void
SVGA_fillrect(PSD psd,COORD x1, COORD y1, COORD x2, COORD y2, PIXELVAL c)
{
	while(y1 <= y2)
		SVGA_drawhline(psd, x1, x2, y1++, c);
}

static void
SVGA_blit(PSD dstpsd,COORD destx,COORD desty,COORD w,COORD h,
		PSD srcpsd,COORD srcx,COORD srcy,int op)
{
	/* FIXME*/
}
