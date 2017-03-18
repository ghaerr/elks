/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 * 
 * MicroWindows Proportional Font Routines (proportional font format)
 *
 * This file contains the generalized low-level font/text
 * drawing routines.  Both fixed and proportional fonts are
 * supported, with fixed pitch structure allowing much smaller
 * font files.
 */
#include <stdio.h>
#include "../device.h"
#include "genfont.h"

/* compiled in fonts*/
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
	&font_winTerminal8x12,		/* FONT_OEM_FIXED*/
	&font_rom8x8			/* FONT_SYSTEM_FIXED*/
#else
	&font_winFreeSystem14x16,	/* FONT_SYSTEM_VAR*/
	&font_winFreeSansSerif11x13,	/* FONT_GUI_VAR*/
	&font_rom8x16,			/* FONT_OEM_FIXED*/
	&font_rom8x8			/* FONT_SYSTEM_FIXED*/
#endif
};

/*
 * Generalized low level get font info routine.  This
 * routine works with fixed and proportional fonts.
 */
BOOL
gen_getfontinfo(PSD psd,FONTID fontid,PFONTINFO pfontinfo)
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
void
gen_gettextsize(PSD psd,const UCHAR *str,int cc,COORD *retwd,COORD *retht,
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
void
gen_gettextbits(PSD psd,UCHAR ch,IMAGEBITS *retmap,COORD *retwd, COORD *retht,
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

#if NOTUSED
/* 
 * Generalized low level text draw routine, called only
 * if no clipping is required
 */
void
gen_drawtext(PSD psd,COORD x,COORD y,const UCHAR *s,int n,PIXELVAL fg,
	FONTID fontid)
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
		psd->GetTextBits(psd, *s++, bitmap, &width, &height, pf);
		gen_drawbitmap(psd, x, y, width, height, bitmap, fg);
		x += width;
	}
}

/*
 * Generalized low level bitmap output routine, called
 * only if no clipping is required.  Only the set bits
 * in the bitmap are drawn, in the foreground color.
 */
void
gen_drawbitmap(PSD psd,COORD x, COORD y, COORD width, COORD height,
	IMAGEBITS *table, PIXELVAL fgcolor)
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
		psd->DrawPixel(psd, x, y, fgcolor);
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
#endif /* NOTUSED*/
