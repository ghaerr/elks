/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver for Linux Framebuffers
 *
 * This file also contains the generalized low-level font/text
 * drawing routines, which will be split out into another file.
 * Both fixed and proportional fonts are supported, with fixed
 * pitch structure allowing much smaller font files.
 */
#include <stdio.h>
#include "../device.h"
#include "fb.h"
#include "genfont.h"

/* specific driver entry points*/
static int  FB_open(PSD psd);
static void FB_close(PSD psd);
static void FB_getscreeninfo(PSD psd,PSCREENINFO psi);
static void FB_setpalette(PSD psd,int first,int count,RGBENTRY *pal);
static void FB_drawpixel(PSD psd,COORD x, COORD y, PIXELVAL c);
static PIXELVAL FB_readpixel(PSD psd,COORD x, COORD y);
static void FB_drawhline(PSD psd,COORD x1, COORD x2, COORD y, PIXELVAL c);
static void FB_drawvline(PSD psd,COORD x, COORD y1, COORD y2, PIXELVAL c);
static void FB_fillrect(PSD psd,COORD x1,COORD y1,COORD x2,COORD y2,PIXELVAL c);
static void FB_blit(PSD dstpsd,COORD destx,COORD desty,COORD w,COORD h,
		PSD srcpsd,COORD srcx,COORD srcy,int op);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	FB_open,
	FB_close,
	FB_getscreeninfo,
	FB_setpalette,
	FB_drawpixel,
	FB_readpixel,
	FB_drawhline,
	FB_drawvline,
	FB_fillrect,
	gen_getfontinfo,
	gen_gettextsize,
	gen_gettextbits,
	FB_blit
};

static int
FB_open(PSD psd)
{
	/* fb_init selects the proper framebuffer subdriver and
	 * inits the SCREENDEVICE struct
	 */
	if (fb_init(psd) == 0)
		return -1;	/* fail*/
	return 1;		/* ok*/
}

static void
FB_close(PSD psd)
{
	fb_exit(psd);
}

static void
FB_getscreeninfo(PSD psd,PSCREENINFO psi)
{
	psi->rows = psd->yres;
	psi->cols = psd->xres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = NUMBER_FONTS;

	if (psd->yres > 480) {
		/* SVGA 800x600*/
		psi->xdpcm = 33;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 33;	/* assumes screen height of 18 cm*/
	} else if (psd->yres > 350) {
		/* VGA 640x480*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
	} else {
		/* EGA 640x350*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
	}
}

/*
 * Set count palette entries starting from first
 */
static void
FB_setpalette(PSD psd,int first,int count,RGBENTRY *pal)
{
	fb_setpalette(psd, first, count, (void*)pal);
}

static void
FB_drawpixel(PSD psd,COORD x, COORD y, PIXELVAL c)
{
	fb_drawpixel(psd, x, y, c);
}

static PIXELVAL
FB_readpixel(PSD psd,COORD x, COORD y)
{
	return fb_readpixel(psd, x, y);
}

static void
FB_drawhline(PSD psd,COORD x1, COORD x2, COORD y, PIXELVAL c)
{
	/*
	 * fb drivers use type 2 line drawing, the last point is not drawn
	 */
	fb_drawhorzline(psd, x1, x2+1, y, c);

	/*
	 * Uncomment the following if driver doesn't support hline
	while (x1 <= x2)
		fb_drawpixel(psd, x1++, y, c);
	 */
}

static void
FB_drawvline(PSD psd,COORD x, COORD y1, COORD y2, PIXELVAL c)
{
	/*
	 * fb drivers use type 2 line drawing, the last point is not drawn
	 */
	fb_drawvertline(psd, x, y1, y2+1, c);

	/*
	 * Uncomment the following if driver doesn't support vline
	while (y1 <= y2)
		fb_drawpixel(psd, x, y1++, c);
	 */
}

static void
FB_fillrect(PSD psd,COORD x1, COORD y1, COORD x2, COORD y2, PIXELVAL c)
{
	/*
	 * Call fb hline (type 2) to save size
	 */
	++x2;		/* fix fb last point not drawn*/
	while (y1 <= y2)
		fb_drawhorzline(psd, x1, x2, y1++, c);
}

static void
FB_blit(PSD dstpsd,COORD destx,COORD desty,COORD w,COORD h,PSD srcpsd,
	COORD srcx,COORD srcy,int op)
{
	fb_blit(dstpsd, destx, desty, w, h, srcpsd, srcx, srcy, op);
}
