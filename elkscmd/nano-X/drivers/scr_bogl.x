/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver using BOGL Library
 *
 * This driver now requires only the following BOGL entry points:
 * 	bogl_init, bogl_done, 
 * 	bogl_pixel, bogl_readpixel,
 * 	bogl_vline, bogl_hline
 *
 * All graphics drawing primitives are based on top of these functions.
 *
 * This file also contains the generalized low-level font/text
 * drawing routines, which will be split out into another file.
 * Both fixed and proportional fonts are supported, with fixed
 * pitch structure allowing much smaller font files.
 */
#include <stdio.h>
#include "../device.h"
#include "bogl/bogl.h"

/* specific bogl driver entry points*/
static int  BOGL_open(SCREENDEVICE *psd);
static void BOGL_close(void);
static void BOGL_getscreeninfo(PSCREENINFO psi);
static void BOGL_setpalette(int first,int count,RGBENTRY *pal);
static void BOGL_drawpixel(COORD x, COORD y, PIXELVAL c);
static PIXELVAL BOGL_readpixel(COORD x, COORD y);
static void BOGL_drawhline(COORD x1, COORD x2, COORD y, PIXELVAL c);
static void BOGL_drawvline(COORD x, COORD y1, COORD y2, PIXELVAL c);
static void BOGL_fillrect(COORD x1, COORD y1, COORD x2, COORD y2, PIXELVAL c);

/* generalized text/font routines*/
static BOOL gen_getfontinfo(FONTID fontid,PFONTINFO pfontinfo);
static void gen_gettextsize(const UCHAR *str,int cc,COORD *retwd,
		COORD *retht,FONTID fontid);
static void gen_gettextbits(UCHAR ch,IMAGEBITS *retmap,COORD *retwd,
		COORD *retht,FONTID fontid);
/*static void gen_drawtext(COORD x, COORD y, const UCHAR *s, int n,
			PIXELVAL fg, FONTID fontid);
static void gen_drawbitmap(COORD x, COORD y, COORD width, COORD height,
			IMAGEBITS *table, PIXELVAL fgcolor);*/

SCREENDEVICE	scrdev = {
	BOGL_open,
	BOGL_close,
	BOGL_getscreeninfo,
	BOGL_setpalette,
	BOGL_drawpixel,
	BOGL_readpixel,
	BOGL_drawhline,
	BOGL_drawvline,
	BOGL_fillrect,
	gen_getfontinfo,
	gen_gettextsize,
	gen_gettextbits,
	640, 480, 256, PF_PALETTE
};

/* compiled in fonts*/
#define NUMBER_FONTS	3
extern FONT font_rom8x16, font_rom8x8;
extern FONT font_winFreeSansSerif11x13;
extern FONT font_winFreeSystem14x16;
extern FONT font_winSystem14x16;
extern FONT font_winMSSansSerif11x13;
extern FONT font_winTerminal8x12;
extern FONT font_helvB10, font_helvB12, font_helvR10;

/* first font is default font*/
PFONT fonts[NUMBER_FONTS] = {
#if HAVEMSFONTS
	&font_winSystem14x16,		/* FONT_SYSTEM_VAR*/
	&font_winMSSansSerif11x13,	/* FONT_GUI_VAR*/
	&font_winTerminal8x12		/* FONT_OEM_FIXED*/
#else
	&font_winFreeSystem14x16,	/* FONT_SYSTEM_VAR*/
	&font_winFreeSansSerif11x13,	/* FONT_GUI_VAR*/
	&font_rom8x16			/* FONT_OEM_FIXED*/
#endif
};

static int
BOGL_open(SCREENDEVICE *psd)
{
	if(bogl_init() == 0)
		return -1;
	psd->xres = bogl_xres;
	psd->yres = bogl_yres;
	psd->ncolors = bogl_ncols;

	/* set pixel format*/
	if(bogl_ncols > 256)
		psd->pixtype = PF_TRUECOLOR24;
	else if(bogl_ncols == 256 && bogl_truecolor)
		psd->pixtype = PF_TRUECOLOR332;
	else
		psd->pixtype = PF_PALETTE;
	return 1;
}

static void
BOGL_close(void)
{
	bogl_done();
}

static void
BOGL_getscreeninfo(PSCREENINFO psi)
{
	psi->rows = scrdev.yres;
	psi->cols = scrdev.xres;
	psi->ncolors = scrdev.ncolors;
	psi->pixtype = scrdev.pixtype;
	psi->black = 0;
	psi->white = 15;
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

/*
 * Set count palette entries starting from first
 */
static void
BOGL_setpalette(int first,int count,RGBENTRY *pal)
{
	bogl_set_palette(first, count, (void*)pal);
}

static void
BOGL_drawpixel(COORD x, COORD y, PIXELVAL c)
{
	bogl_pixel(x, y, c);
}

static PIXELVAL
BOGL_readpixel(COORD x, COORD y)
{
	return bogl_readpixel(x, y);
}

static void
BOGL_drawhline(COORD x1, COORD x2, COORD y, PIXELVAL c)
{
	/*
	 * bogl uses type 2 line drawing, the last point is not drawn
	 */
	bogl_hline(x1, x2+1, y, c);

	/*
	 * Uncomment the following if driver doesn't support hline
	while(x1 <= x2)
		bogl_pixel(x1++, y, c);
	 */
}

static void
BOGL_drawvline(COORD x, COORD y1, COORD y2, PIXELVAL c)
{
	/*
	 * bogl uses type 2 line drawing, the last point is not drawn
	 */
	bogl_vline(x, y1, y2+1, c);

	/*
	 * Uncomment the following if driver doesn't support vline
	while(y1 <= y2)
		bogl_pixel(x, y1++, c);
	 */
}

static void
BOGL_fillrect(COORD x1, COORD y1, COORD x2, COORD y2, PIXELVAL c)
{
	/*
	 * Call bogl hline (type 2) to save size
	 */
	++x2;		/* fix bogl last point not drawn*/
	while(y1 <= y2)
		bogl_hline(x1, x2, y1++, c);
	/*
	 * Uncomment the following if driver doesn't support fillrect
	while(y1 <= y2)
		BOGL_drawhline(x1, x2, y1++, c);
	 */
}

#if 0000
/* 
 * Generalized low level text draw routine, called only
 * if no clipping is required
 */
static void
gen_drawtext(COORD x,COORD y,const UCHAR *s,int n,PIXELVAL fg,FONTID fontid)
{
	COORD 		width;			/* width of character */
	COORD 		height;			/* height of character */
	PFONT		pf;
	IMAGEBITS 	bitmap[MAX_CHAR_HEIGHT];/* bitmap for character */

	if(fontid >= NUMBER_FONTS)
		return;
	pf = fonts[fontid];

	/* x, y is bottom left corner*/
	y -= pf->height - 1;
	while (n-- > 0) {
		gen_gettextbits(*s++, bitmap, &width, &height, pf);
		gen_drawbitmap(x, y, width, height, bitmap, fg);
		x += width;
	}
}

/*
 * Generalized low level bitmap output routine, called
 * only if no clipping is required.  Only the set bits
 * in the bitmap are drawn, in the foreground color.
 */
static void
gen_drawbitmap(COORD x, COORD y, COORD width, COORD height, IMAGEBITS *table,
	PIXELVAL fgcolor)
{
  COORD minx;
  COORD maxx;
  IMAGEBITS bitvalue;	/* bitmap word value */
  int bitcount;			/* number of bits left in bitmap word */

  minx = x;
  maxx = x + width - 1;
  bitcount = 0;
  while (height > 0) {
	if (bitcount <= 0) {
		bitcount = IMAGE_BITSPERIMAGE;
		bitvalue = *table++;
	}
	if (IMAGE_TESTBIT(bitvalue))
		BOGL_drawpixel(x, y, fgcolor);
	bitvalue = IMAGE_SHIFTBIT(bitvalue);
	--bitcount;
	if (x++ == maxx) {
		x = minx;
		++y;
		--height;
		bitcount = 0;
	}
  }
}
#endif

/*
 * Generalized low level get font info routine.  This
 * routine works with fixed and proportional fonts.
 */
static BOOL
gen_getfontinfo(FONTID fontid,PFONTINFO pfontinfo)
{
	PFONT	pf;
	int	i;

	if(fontid >= NUMBER_FONTS)
		return FALSE;
	pf = fonts[fontid];

	pfontinfo->font = fontid;
	pfontinfo->height = pf->height;
	pfontinfo->maxwidth = pf->maxwidth;
	pfontinfo->baseline = 0;
	pfontinfo->fixed = pf->width == NULL? TRUE: FALSE;
	for(i=0; i<256; ++i) {
		if(pf->width == NULL)
			pfontinfo->widths[i] = pf->maxwidth;
		else {
			if(i<pf->firstchar || i >= pf->firstchar+pf->size)
				pfontinfo->widths[i] = 0;
			else pfontinfo->widths[i] = pf->width[i-pf->firstchar];
		}
	}
	return TRUE;
}

/*
 * Generalized low level routine to calc bounding box for text output.
 * Handles both fixed and proportional fonts.
 */
static void
gen_gettextsize(const UCHAR *str,int cc,COORD *retwd,COORD *retht,
	FONTID fontid)
{
	PFONT	pf;
	int	c;
	int	width;

	if(fontid >= NUMBER_FONTS) {
		*retht = 0;
		*retwd = 0;
		return;
	}
	pf = fonts[fontid];

	if(pf->width == NULL)
		width = cc * pf->maxwidth;
	else {
		width = 0;
		while(--cc >= 0) {
			if( (c = *str++) >= pf->firstchar &&
			     c < pf->firstchar+pf->size)
				width += pf->width[c - pf->firstchar];
		}
	}

	*retwd = width;
	*retht = pf->height;

}

/*
 * Generalized low level routine to get the bitmap associated
 * with a character.  Handles fixed and proportional fonts.
 */
static void
gen_gettextbits(UCHAR ch,IMAGEBITS *retmap,COORD *retwd, COORD *retht,
	FONTID fontid)
{
	int		n;
	PFONT		pf = NULL;
	IMAGEBITS *	bits;

	if(fontid < NUMBER_FONTS)
		pf = fonts[fontid];

	if(!pf || ch < pf->firstchar || ch >= pf->firstchar+pf->size) {
		*retht = 0;
		*retwd = 0;
		return;
	}
	ch -= pf->firstchar;

	/* get font bitmap depending on fixed pitch or not*/
	bits = pf->bits + (pf->offset? pf->offset[ch]: (pf->height * ch));
	for(n=0; n<pf->height; ++n)
		*retmap++ = *bits++;

	/* return width depending on fixed pitch or not*/
	*retwd = pf->width? pf->width[ch]: pf->maxwidth;
	*retht = pf->height;
}

#if 0
static unsigned char palette[16+16][3] = {
  /* Linux 16 color palette*/
  { 0x00,0x00,0x00 },
  { 0x00,0x00,0xaa },
  { 0x00,0xaa,0x00 },
  { 0x00,0xaa,0xaa },
  { 0xaa,0x00,0x00 },
  { 0xaa,0x00,0xaa },
  { 0xaa,0x55,0x00 },	/* adjust to brown*/
  //{ 0xaa,0xaa,0x00 },
  { 0xaa,0xaa,0xaa },
  { 0x55,0x55,0x55 },
  { 0x55,0x55,0xff },
  { 0x55,0xff,0x55 },
  { 0x55,0xff,0xff },
  { 0xff,0x55,0x55 },
  { 0xff,0x55,0xff },
  { 0xff,0xff,0x55 },
  { 0xff,0xff,0xff },

  /* 16 entry std palette*/
  {0x00, 0x00, 0x00},
  {0x00, 0x00, 0xbf},
  {0x00, 0xbf, 0x00},
  {0x00, 0xbf, 0xbf},
  {0xbf, 0x00, 0x00},
  {0xbf, 0x00, 0xbf},
  {0xbf, 0x60, 0x00},	/* adjust to brown*/
  //{0xbf, 0xbf, 0x00},
  {0xc0, 0xc0, 0xc0},
  {0x80, 0x80, 0x80},
  {0x00, 0x00, 0xff},
  {0x00, 0xff, 0x00},
  {0x00, 0xff, 0xff},
  {0xff, 0x00, 0x00},
  {0xff, 0x00, 0xff},
  {0xff, 0xff, 0x00},
  {0xff, 0xff, 0xff},
};
#endif
