/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 * 
 * PC ROM Font Routine Header (PC ROM font format)
 *
 * This file contains the PC ROM format low-level font/text
 * drawing routines.  Only fixed pitch fonts are supported.
 * The ROM character matrix is used for the text bitmaps.
 *
 * The environment variable CHARHEIGHT if set will set the assumed rom
 * font character height, which defaults to 14.
 */
#include <stdlib.h>
#include "../device.h"
#include "vgaplan4.h"
#include "romfont.h"

/* local data*/
int	ROM_CHAR_HEIGHT;	/* number of scan lines in fonts in ROM */
FARADDR rom_char_addr;

/* init PC ROM routines, must be called in graphics mode*/
void
pcrom_init(PSD psd)
{
	char *	p;
	FARADDR rom_char_addr_temp;

	/* use INT 10h to get address of rom character table*/

	/* we first compare of 8x14 fonts address is equal
	 * to the 8x16 fonts address. If yes, it means
	 * the 8x14 are not present, so we fallback to
	 * 8x16 fonts. Source:
	 * https://www.bttr-software.de/products/fix8x14/
	 */

	rom_char_addr = int10(FNGETROMADDR, GETROM8x14);
	rom_char_addr_temp = int10(FNGETROMADDR, GETROM8x16);

	if (rom_char_addr == rom_char_addr_temp) {
		ROM_CHAR_HEIGHT = 16;
		rom_char_addr = rom_char_addr_temp;
	}
	else {
		ROM_CHAR_HEIGHT = 14;
	}

#if 0
	/* check bios data area for actual character height,
	 * as the returned font isn't always 14 high
	 */
	ROM_CHAR_HEIGHT = GETBYTE_FP(MK_FP(0x0040, 0x0085));
	if(ROM_CHAR_HEIGHT > MAX_ROM_HEIGHT)
		ROM_CHAR_HEIGHT = MAX_ROM_HEIGHT;
#endif

	p = getenv("CHARHEIGHT");
	if(p)
		ROM_CHAR_HEIGHT = atoi(p);
}

/*
 * PC ROM low level get font info routine.  This routine
 * returns info on a single bios ROM font.
 */
BOOL
pcrom_getfontinfo(PSD psd,FONTID fontid,PFONTINFO pfontinfo)
{
	int	i;

	pfontinfo->font = 0;
	pfontinfo->height = ROM_CHAR_HEIGHT;
	pfontinfo->maxwidth = ROM_CHAR_WIDTH;
	pfontinfo->baseline = 3;
	pfontinfo->fixed = TRUE;
	for (i = 0; i < 256; i++)
		pfontinfo->widths[i] = ROM_CHAR_WIDTH;
	return TRUE;
}

/*
 * PC ROM low level routine to calc bounding box for text output.
 * Handles bios ROM font only.
 */
void
pcrom_gettextsize(PSD psd,const UCHAR *str,int cc,COORD *retwd,COORD *retht,
	FONTID fontid)
{
	*retwd = ROM_CHAR_WIDTH * cc;
	*retht = ROM_CHAR_HEIGHT;
}

/*
 * PC ROM low level routine to get the bitmap associated
 * with a character.  Handles bios ROM font only.
 */
void
pcrom_gettextbits(PSD psd,UCHAR ch,IMAGEBITS *retmap,COORD *retwd, COORD *retht,
	FONTID fontid)
{
	FARADDR	bits;
	int	n;

	/* read character bits from rom*/
	bits = rom_char_addr + ch * ROM_CHAR_HEIGHT;
	for(n=0; n<ROM_CHAR_HEIGHT; ++n)
		*retmap++ = GETBYTE_FP(bits++) << 8;

	*retwd = ROM_CHAR_WIDTH;
	*retht = ROM_CHAR_HEIGHT;
}

#if NOTUSED
/* 
 * Low level text draw routine, called only if no clipping
 * is required.  This routine draws ROM font characters only.
 */
void
pcrom_drawtext(PSD psd,COORD x,COORD y,const UCHAR *s,int n,PIXELVAL fg,
	FONTID fontid)
{
	COORD 		width;			/* width of character */
	COORD 		height;			/* height of character */
	IMAGEBITS 	bitmap[MAX_ROM_HEIGHT];	/* bitmap for character */

 	/* x,y is bottom left corner*/
	y -= (ROM_CHAR_HEIGHT - 1);
	while (n-- > 0) {
		psd->GetTextBits(psd, *s++, bitmap, &width, &height, pfont);
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
