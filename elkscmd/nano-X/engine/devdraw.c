/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Device-independent mid level drawing and color routines.
 *
 * These routines do the necessary range checking, clipping, and cursor
 * overwriting checks, and then call the lower level device dependent
 * routines to actually do the drawing.  The lower level routines are
 * only called when it is known that all the pixels to be drawn are
 * within the device area and are visible.
 */
/*#define NDEBUG*/
#include <stdio.h>
/*#include <assert.h>*/
#include "device.h"

/*
 * The following define can change depending on the window manager
 * usage of colors and layout of the 8bpp palette devpal8.c.
 * Color entries below this value won't be overwritten by user
 * programs or bitmap display conversion tables.
 */
#define FIRSTUSERPALENTRY	24  /* first writable pal entry over 16 color*/

static PIXELVAL gr_foreground;	    /* current foreground color */
static PIXELVAL gr_background;	    /* current background color */
static BOOL 	gr_usebg;    	    /* TRUE if background drawn in pixmaps */
       MODE 	gr_mode; 	    /* drawing mode */
static FONTID	gr_font;	    /* current font, 0 for default font*/
static SCREENINFO gr_sinfo;	    /* screen info for local routines*/
static RGBENTRY	gr_palette[256];    /* current palette*/
static int	gr_firstuserpalentry;/* first user-changable palette entry*/
static int 	gr_nextpalentry;    /* next available palette entry*/

static void drawpoint(PSD psd,COORD x, COORD y);
static void drawrow(PSD psd,COORD x1,COORD x2,COORD y);
static void drawcol(PSD psd,COORD x,COORD y1,COORD y2);
static void draw4points(PSD psd,COORD x,COORD y,COORD px,COORD py);
static void extendrow(COORD y,COORD x1,COORD y1,COORD x2,COORD y2,
		COORD *minxptr,COORD *maxxptr);

/*
 * Open low level graphics driver
 */
int
GdOpenScreen(void)
{
	RGBENTRY *	stdpal;
	extern RGBENTRY	stdpal1[2];
	extern RGBENTRY	stdpal2[4];
	extern RGBENTRY	stdpal4[16];
	/*extern RGBENTRY	stdpal8[256];*/

	if(scrdev.Open(&scrdev) < 0)
		return -1;
	GdGetScreenInfo(&scrdev, &gr_sinfo);

	/* assume no user changable palette entries*/
	gr_firstuserpalentry = (int)gr_sinfo.ncolors;

	/* set palette according to system colors and devpalX.c*/
	switch((int)gr_sinfo.ncolors) {
	case 2:		/* 1bpp*/
		stdpal = stdpal1;
		break;
	case 4:		/* 2bpp*/
		stdpal = stdpal2;
		break;
	case 8:		/* 3bpp - not fully supported*/
	case 16:	/* 4bpp*/
		stdpal = stdpal4;
		break;
#if FRAMEBUFFER	/* don't require large stdpal8 if not framebuffer system*/
	case 256:	/* 8bpp*/
		/* start after last system-reserved color*/
		gr_firstuserpalentry = FIRSTUSERPALENTRY;
		stdpal = stdpal8;
		break;
#endif
	default:	/* truecolor*/
		/* no palette*/
		gr_firstuserpalentry = 0;
		stdpal = NULL;
	}

	/* reset next user palette entry, write hardware palette*/
	GdResetPalette();
	GdSetPalette(0, (int)gr_sinfo.ncolors, stdpal);

	/* init local vars*/
	GdSetMode(MODE_SET);
	GdSetForeground(GdFindColor(WHITE));
	GdSetBackground(GdFindColor(BLACK));
	GdSetUseBackground(TRUE);
	GdSetFont(0);
	GdSetClipRects(0, NULL);

	/* fill black (actually fill to first palette entry or truecolor 0*/
	scrdev.FillRect(&scrdev, 0, 0, gr_sinfo.cols-1, gr_sinfo.rows-1, 0);
	return 0;
}

/*
 * Close low level graphics driver
 */
void 
GdCloseScreen(void)
{
#if 0
	/* take screenshot on exit*/
	int n, ifd, ofd;
	long i;
	char buf[256];

	ifd = open("/dev/fb0", 0);
	ofd = creat("file", 0666);
	for(n=0; n<256; ++n)
		write(ofd, &gr_palette[n], 3);
	for(i=gr_sinfo.cols*gr_sinfo.rows; i > 0; ) {
		if((n = read(ifd, buf, 256)) > 0) {
			write(ofd, buf, n);
			i -= n;
		}
	}
	close(ifd);
	close(ofd);
#endif
	scrdev.Close(&scrdev);
}

/*
 * Set the drawing mode for future calls.
 */
MODE
GdSetMode(MODE m)
{
	MODE	oldmode = gr_mode;

	gr_mode = m;
	return oldmode;
}

/*
 * Set whether or not the background is used for drawing pixmaps and text.
 */
BOOL
GdSetUseBackground(BOOL flag)
{
	BOOL	oldusebg = gr_usebg;

	gr_usebg = flag;
	return oldusebg;
}

/*
 * Set the foreground color for drawing.
 */
PIXELVAL
GdSetForeground(PIXELVAL fg)
{
	PIXELVAL	oldfg = gr_foreground;

	gr_foreground = fg;
	return oldfg;
}

/*
 * Set the background color for bitmap and text backgrounds.
 */
PIXELVAL
GdSetBackground(PIXELVAL bg)
{
	PIXELVAL	oldbg = gr_background;

	gr_background = bg;
	return oldbg;
}

/*
 * Set the font for future calls.
 */
FONTID
GdSetFont(FONTID fontid)
{
	FONTID	oldfont = gr_font;

	gr_font = fontid;
	return oldfont;
}

/* reset palette to empty except for system colors*/
void
GdResetPalette(void)
{
	/* note: when palette entries are changed, all 
	 * windows may need to be redrawn
	 */
	gr_nextpalentry = gr_firstuserpalentry;
}

/* set the system palette section to the passed palette entries*/
void
GdSetPalette(int first, int count, RGBENTRY *palette)
{
	int	i;

	/* no palette management needed if running truecolor*/
	if(gr_sinfo.pixtype != PF_PALETTE)
		return;

	/* bounds check against # of device color entries*/
	if(first + count > (int)gr_sinfo.ncolors)
		count = (int)gr_sinfo.ncolors - first;
	if(first < (int)gr_sinfo.ncolors)
		scrdev.SetPalette(&scrdev, first, count, palette);

	/* copy palette for GdFind*Color*/
	for(i=0; i<count; ++i)
		gr_palette[i+first] = palette[i];
}

/*
 * Convert a palette-independent value to a hardware color
 */
PIXELVAL
GdFindColor(COLORVAL c)
{
	/*
	 * Handle truecolor displays.  Note that the F_PALINDEX
	 * bit is ignored when running truecolor drivers.
	 */
	switch(gr_sinfo.pixtype) {
	case PF_TRUECOLOR24:
		/* create 24 bit pixel from RGB colorval*/
		return (PIXELVAL)c & 0x00ffffff;

	case PF_TRUECOLOR565:
		/* create 16 bit 5/6/5 format pixel from RGB colorval */
		return  ((REDVALUE(c) & 0xf8) << 8) |
			((GREENVALUE(c) & 0xfc) << 3) |
			((BLUEVALUE(c) & 0xf8) >> 3);

	case PF_TRUECOLOR332:
		/* create 8 bit 3/3/2 format pixel from RGB colorval*/
		return  (REDVALUE(c) & 0xe0) |
			((GREENVALUE(c) & 0xe0) >> 3) |
			((BLUEVALUE(c) & 0xc0) >> 6);
	}

	/* case PF_PALETTE: must be running 1, 2, 4 or 8 bit palette*/

	/*
	 * Check if color is a palette index.  Note that the index
	 * isn't error checked against the system palette, for speed.
	 */
	if(c & F_PALINDEX)
		return (c & 0xff);

	/* search palette for closest match*/
	return GdFindNearestColor(gr_palette, (int)gr_sinfo.ncolors, c);
}

/*
 * Search a palette to find the nearest color requested.
 * Uses a weighted squares comparison.
 */
PIXELVAL
GdFindNearestColor(RGBENTRY *pal, int size, COLORVAL cr)
{
	RGBENTRY *	rgb;
	int		r, g, b;
	int		R, G, B;
	long		diff = 0x7fffffffL;
	long		sq;
	int		best = 0;

	r = REDVALUE(cr);
	g = GREENVALUE(cr);
	b = BLUEVALUE(cr);
	for(rgb=pal; diff && rgb < &pal[size]; ++rgb) {
		R = rgb->r - r;
		G = rgb->g - g;
		B = rgb->b - b;
		sq = (long)R*R*30*30 + (long)G*G*59*59 + (long)B*B*11*11;

		if(sq < diff) {
			best = rgb - pal;
			if((diff = sq) == 0)
				return best;
		}
	}
	return best;
}

/*
 * Return about the screen.
 */
void
GdGetScreenInfo(PSD psd, PSCREENINFO psi)
{
	psd->GetScreenInfo(psd, psi);
	GdGetButtonInfo(&psi->buttons);
	GdGetModifierInfo(&psi->modifiers);
}

/*
 * Return information about a specified font.
 */
BOOL
GdGetFontInfo(PSD psd, FONTID fontid, PFONTINFO pfontinfo)
{
	return psd->GetFontInfo(psd, fontid, pfontinfo);
}

/*
 * Draw a point using the current clipping region and foreground color.
 */
void
GdPoint(PSD psd, COORD x, COORD y)
{
	if (GdClipPoint(x, y)) {
		psd->DrawPixel(psd, x, y, gr_foreground);
		GdFixCursor();
	}
}

/*
 * Draw an arbitrary line using the current clipping region and foreground color
 * If bDrawLastPoint is FALSE, draw up to but not including point x2, y2.
 *
 * This routine is the only routine that adjusts coordinates for supporting
 * two different types of upper levels, those that draw the last point
 * in a line, and those that draw up to the last point.  All other local
 * routines draw the last point.  This gives this routine a bit more overhead,
 * but keeps overall complexity down.
 */
void
GdLine(PSD psd, COORD x1, COORD y1, COORD x2, COORD y2, BOOL bDrawLastPoint)
{
  int xdelta;			/* width of rectangle around line */
  int ydelta;			/* height of rectangle around line */
  int xinc;			/* increment for moving x coordinate */
  int yinc;			/* increment for moving y coordinate */
  int rem;			/* current remainder */
  COORD temp;

  /* See if the line is horizontal or vertical. If so, then call
   * special routines.
   */
  if (y1 == y2) {
	/*
	 * Adjust coordinates if not drawing last point.  Tricky.
	 */
	if(!bDrawLastPoint) {
		if (x1 > x2) {
			temp = x1;
			x1 = x2 + 1;
			x2 = temp;
		} else
			--x2;
	}

	/* call faster line drawing routine*/
	drawrow(psd, x1, x2, y1);
	GdFixCursor();
	return;
  }
  if (x1 == x2) {
	/*
	 * Adjust coordinates if not drawing last point.  Tricky.
	 */
	if(!bDrawLastPoint) {
		if (y1 > y2) {
			temp = y1;
			y1 = y2 + 1;
			y2 = temp;
		} else
			--y2;
	}

	/* call faster line drawing routine*/
	drawcol(psd, x1, y1, y2);
	GdFixCursor();
	return;
  }

  /* See if the line is either totally visible or totally invisible. If
   * so, then the line drawing is easy.
   */
  switch (GdClipArea(x1, y1, x2, y2)) {
      case CLIP_VISIBLE:
	/*
	 * For size considerations, there's no low-level bresenham
	 * line draw, so we've got to draw all non-vertical
	 * and non-horizontal lines with per-point
	 * clipping for the time being
	psd->Line(psd, x1, y1, x2, y2, gr_foreground);
	GdFixCursor();
	return;
	 */
	break;
      case CLIP_INVISIBLE:
	return;
  }

  /* The line may be partially obscured. Do the draw line algorithm
   * checking each point against the clipping regions.
   */
  xdelta = x2 - x1;
  ydelta = y2 - y1;
  if (xdelta < 0) xdelta = -xdelta;
  if (ydelta < 0) ydelta = -ydelta;
  xinc = (x2 > x1) ? 1 : -1;
  yinc = (y2 > y1) ? 1 : -1;
  if (GdClipPoint(x1, y1))
	  psd->DrawPixel(psd, x1, y1, gr_foreground);
  if (xdelta >= ydelta) {
	rem = xdelta / 2;
	for(;;) {
		if(!bDrawLastPoint && x1 == x2)
			break;
		x1 += xinc;
		rem += ydelta;
		if (rem >= xdelta) {
			rem -= xdelta;
			y1 += yinc;
		}
		if (GdClipPoint(x1, y1))
			psd->DrawPixel(psd, x1, y1, gr_foreground);
		if(bDrawLastPoint && x1 == x2)
			break;
	}
  } else {
	rem = ydelta / 2;
	for(;;) {
		if(!bDrawLastPoint && y1 == y2)
			break;
		y1 += yinc;
		rem += xdelta;
		if (rem >= ydelta) {
			rem -= ydelta;
			x1 += xinc;
		}
		if (GdClipPoint(x1, y1))
			psd->DrawPixel(psd, x1, y1, gr_foreground);
		if(bDrawLastPoint && y1 == y2)
			break;
	}
  }
  GdFixCursor();
}

/* Draw a point in the foreground color, applying clipping if necessary*/
static void
drawpoint(PSD psd, COORD x, COORD y)
{
	if (GdClipPoint(x, y))
		psd->DrawPixel(psd, x, y, gr_foreground);
}

/* Draw a horizontal line from x1 to and including x2 in the
 * foreground color, applying clipping if necessary.
 */
static void
drawrow(PSD psd, COORD x1,COORD x2,COORD y)
{
  COORD temp;

  /* reverse endpoints if necessary*/
  if (x1 > x2) {
	temp = x1;
	x1 = x2;
	x2 = temp;
  }

  /* clip to physical device*/
  if (x1 < 0)
	  x1 = 0;
  if (x2 >= psd->xres)
	  x2 = psd->xres - 1;

  /* check cursor intersect once for whole line*/
  GdCheckCursor(x1, y, x2, y);

  while (x1 <= x2) {
	if (GdClipPoint(x1, y)) {
		temp = min(clipmaxx, x2);
		psd->DrawHorzLine(psd, x1, temp, y, gr_foreground);
	} else
		temp = min(clipmaxx, x2);
	x1 = temp + 1;
  }
}

/* Draw a vertical line from y1 to and including y2 in the
 * foreground color, applying clipping if necessary.
 */
static void
drawcol(PSD psd, COORD x,COORD y1,COORD y2)
{
  COORD temp;

  /* reverse endpoints if necessary*/
  if (y1 > y2) {
	temp = y1;
	y1 = y2;
	y2 = temp;
  }

  /* clip to physical device*/
  if (y1 < 0)
	  y1 = 0;
  if (y2 >= psd->yres)
	  y2 = psd->yres - 1;

  /* check cursor intersect once for whole line*/
  GdCheckCursor(x, y1, x, y2);

  while (y1 <= y2) {
	if (GdClipPoint(x, y1)) {
		temp = min(clipmaxy, y2);
		psd->DrawVertLine(psd, x, y1, temp, gr_foreground);
	} else
		temp = min(clipmaxy, y2);
	y1 = temp + 1;
  }
}

/* Draw a rectangle in the foreground color, applying clipping if necessary.
 * This is careful to not draw points multiple times in case the rectangle
 * is being drawn using XOR.
 */
void
GdRect(PSD psd, COORD x, COORD y, COORD width, COORD height)
{
  COORD maxx;
  COORD maxy;

  if (width <= 0 || height <= 0)
	  return;
  maxx = x + width - 1;
  maxy = y + height - 1;
  drawrow(psd, x, maxx, y);
  if (height > 1)
	  drawrow(psd, x, maxx, maxy);
  if (height < 3)
	  return;
  y++;
  maxy--;
  drawcol(psd, x, y, maxy);
  if (width > 1)
	  drawcol(psd, maxx, y, maxy);
  GdFixCursor();
}

/* Draw a filled in rectangle in the foreground color, applying
 * clipping if necessary.
 */
void
GdFillRect(PSD psd, COORD x1, COORD y1, COORD width, COORD height)
{
  COORD x2 = x1+width-1;
  COORD y2 = y1+height-1;

  /* See if the rectangle is either totally visible or totally
   * invisible. If so, then the rectangle drawing is easy.
   */
  switch (GdClipArea(x1, y1, x2, y2)) {
      case CLIP_VISIBLE:
	psd->FillRect(psd, x1, y1, x2, y2, gr_foreground);
	GdFixCursor();
	return;

      case CLIP_INVISIBLE:
	return;
  }

  /* The rectangle may be partially obstructed. So do it line by line. */
  while (y1 <= y2)
	  drawrow(psd, x1, x2, y1++);
  GdFixCursor();
}

/* Get the width and height of passed text string in the current font*/
void
GdGetTextSize(PSD psd, char *str, int cc, COORD *pwidth, COORD *pheight)
{
	psd->GetTextSize(psd, (unsigned char *)str, cc, pwidth, pheight, gr_font);
}

/* Draw a text string at a specifed coordinates in the foreground color
 * (and possibly the background color), applying clipping if necessary.
 * The background color is only drawn if the gr_usebg flag is set.
 */
void
GdText(PSD psd, COORD x, COORD y, const UCHAR *str, int cc, BOOL fBottomAlign)
{
	COORD		width;			/* width of text area */
	COORD 		height;			/* height of text area */
	IMAGEBITS 	bitmap[MAX_CHAR_HEIGHT];/* bitmaps for characters */

	if (cc <= 0)
		return;

	psd->GetTextSize(psd, str, cc, &width, &height, gr_font);
	
	if(fBottomAlign)
		y -= (height - 1);

	switch (GdClipArea(x, y, x + width - 1, y + height - 1)) {
	case CLIP_VISIBLE:
		/*
		 * For size considerations, there's no low-level text
		 * draw, so we've got to draw all text
		 * with per-point clipping for the time being
		if (gr_usebg)
			psd->FillRect(psd, x, y, x + width - 1, y + height - 1,
				gr_background);
		psd->DrawText(psd, x, y, str, cc, gr_foreground, gr_font);
		GdFixCursor();
		return;
		*/
		break;

	case CLIP_INVISIBLE:
		return;
	}

	/* Get the bitmap for each character individually, and then display
	 * them using clipping for each one.
	 */
	while (cc-- > 0 && x < psd->xres) {
		psd->GetTextBits(psd, *str++, bitmap, &width, &height, gr_font);
		/* note: change to bitmap*/
		GdBitmap(psd, x, y, width, height, bitmap);
		x += width;
	}
	GdFixCursor();
}

/*
 * Draw a rectangular area using the current clipping region and the
 * specified bit map.  This differs from rectangle drawing in that the
 * rectangle is drawn using the foreground color and possibly the background
 * color as determined by the bit map.  Each row of bits is aligned to the
 * next bitmap word boundary (so there is padding at the end of the row).
 * The background bit values are only written if the gr_usebg flag
 * is set.
 */
void
GdBitmap(PSD psd, COORD x, COORD y, COORD width, COORD height,
	IMAGEBITS *imagebits)
{
  COORD minx;
  COORD maxx;
  PIXELVAL savecolor;		/* saved foreground color */
  IMAGEBITS bitvalue = 0;	/* bitmap word value */
  int bitcount;			/* number of bits left in bitmap word */

  switch (GdClipArea(x, y, x + width - 1, y + height - 1)) {
      case CLIP_VISIBLE:
	/*
	 * For size considerations, there's no low-level bitmap
	 * draw so we've got to draw everything with per-point
	 * clipping for the time being.
	if (gr_usebg)
		psd->FillRect(psd, x, y, x + width - 1, y + height - 1,
			gr_background);
	psd->DrawBitmap(psd, x, y, width, height, imagebits, gr_foreground);
	return;
	*/
	break;

      case CLIP_INVISIBLE:
	return;
  }

  /* The rectangle is partially visible, so must do clipping. First
   * fill a rectangle in the background color if necessary.
   */
  if (gr_usebg) {
	savecolor = gr_foreground;
	gr_foreground = gr_background;
	/* note: change to fillrect*/
	GdFillRect(psd, x, y, width, height);
	gr_foreground = savecolor;
  }
  minx = x;
  maxx = x + width - 1;
  bitcount = 0;
  while (height > 0) {
	if (bitcount <= 0) {
		bitcount = IMAGE_BITSPERIMAGE;
		bitvalue = *imagebits++;
	}
	if (IMAGE_TESTBIT(bitvalue) && GdClipPoint(x, y))
		psd->DrawPixel(psd, x, y, gr_foreground);
	bitvalue = IMAGE_SHIFTBIT(bitvalue);
	bitcount--;
	if (x++ == maxx) {
		x = minx;
		y++;
		height--;
		bitcount = 0;
	}
  }
  GdFixCursor();
}

/*
 * Return true if color is in palette
 */
BOOL
GdColorInPalette(COLORVAL cr,RGBENTRY *palette,int palsize)
{
	int	i;

	for(i=0; i<palsize; ++i)
		if(GETPALENTRY(palette, i) == cr)
			return TRUE;
	return FALSE;
}

/*
 * Create a PIXELVAL conversion table between the passed palette
 * and the in-use palette.  The system palette is loaded/merged according
 * to fLoadType.
 */
void
GdMakePaletteConversionTable(RGBENTRY *palette,int palsize,PIXELVAL *convtable,
	int fLoadType)
{
	int		i;
	COLORVAL	cr;
	int		newsize, nextentry;
	RGBENTRY	newpal[256];

	/*
	 * Check for load palette completely, or add colors
	 * from passed palette to system palette until full.
	 */
	if(gr_sinfo.pixtype == PF_PALETTE) {
	    switch(fLoadType) {
	    case LOADPALETTE:
		/* Load palette from beginning with image's palette.
		 * First palette entries are micro-windows colors
		 * and not changed.
		 */
		GdSetPalette(gr_firstuserpalentry, palsize, palette);
		break;

	    case MERGEPALETTE:
		/* get system palette*/
		for(i=0; i<(int)gr_sinfo.ncolors; ++i)
			newpal[i] = gr_palette[i];

		/* merge passed palette into system palette*/
		newsize = 0;
		nextentry = gr_nextpalentry;

		/* if color missing and there's room, add it*/
		for(i=0; i<palsize && nextentry < (int)gr_sinfo.ncolors; ++i) {
			cr = GETPALENTRY(palette, i);
			if(!GdColorInPalette(cr, newpal, nextentry)) {
				newpal[nextentry++] = palette[i];
				++newsize;
			}
		}

		/* set the new palette if any color was added*/
		if(newsize) {
			GdSetPalette(gr_nextpalentry, newsize,
				&newpal[gr_nextpalentry]);
			gr_nextpalentry += newsize;
		}
		break;
	    }
	}

	/*
	 * Build conversion table from inuse system palette and
	 * passed palette.  This will load RGB values directly
	 * if running truecolor, otherwise it will find the
	 * nearest color from the inuse palette.
	 * FIXME: tag the conversion table to the bitmap image
	 */
	for(i=0; i<palsize; ++i) {
		cr = GETPALENTRY(palette, i);
		convtable[i] = GdFindColor(cr);
	}
}

/*
 * Draw a color bitmap image in 1, 4 or 8 bits per pixel.  The
 * Micro-Win color image format are DWORD padded bytes, with
 * the upper bits corresponding to the left side (identical to 
 * the MS-Windows format).  This format is currently different
 * than the IMAGEBITS format, which uses word-padded bits
 * for monochrome display only, where the upper bits in the word
 * correspond with the left side.
 */
void
GdDrawImage(PSD psd, COORD x, COORD y, PIMAGEHDR pimage)
{
  COORD minx;
  COORD maxx;
  UCHAR bitvalue = 0;
  int bitcount;
  UCHAR *imagebits;
  COORD	height, width;
  PIXELVAL pixel;
  int clip;
  int extra, linesize;
  COORD yoff;
  PIXELVAL convtable[256];

  height = pimage->height;
  width = pimage->width;

  /* determine if entire image is clipped out, save clipresult for later*/
  clip = GdClipArea(x, y, x + width - 1, y + height - 1);
  if(clip == CLIP_INVISIBLE)
	return;

  /*
   * Merge the images's palette and build a palette index conversion table.
   */
  GdMakePaletteConversionTable(pimage->palette, pimage->palsize, convtable,
	MERGEPALETTE);

  minx = x;
  maxx = x + width - 1;
  imagebits = pimage->imagebits;

  if(pimage->compression & 01) {
	/* bottom-up dibs*/
	y += height - 1;
	yoff = -1;
  } else
	yoff = 1;

#define PIX2BYTES(n)	(((n)+7)/8)
  /* imagebits are dword aligned*/
  switch(pimage->bpp) {
  default:
  case 8:
	linesize = width;
	break;
  case 4:
	linesize = PIX2BYTES(width<<2);
	break;
  case 1:
	linesize = PIX2BYTES(width);
	break;
  }
  extra = ((linesize + 3) & ~3) - linesize;

  bitcount = 0;
  while(height > 0) {
	if (bitcount <= 0) {
		bitcount = sizeof(UCHAR) * 8;
		bitvalue = *imagebits++;
	}
	switch(pimage->bpp) {
	default:
	case 8:
		pixel = bitvalue;
		bitcount = 0;
		break;
	case 4:
		pixel = (bitvalue & 0xf0) >> 4;
		bitvalue <<= 4;
		bitcount -= 4;
		break;
	case 1:
		pixel = (bitvalue & 0x80)? 1: 0;
		bitvalue <<= 1;
		--bitcount;
		break;
	}

	if(clip == CLIP_VISIBLE || GdClipPoint(x, y)) {
		psd->DrawPixel(psd, x, y, convtable[pixel]);
	}
#if 0
	// fix: use clipmaxx to clip quicker
	else if(clip != CLIP_VISIBLE && !clipresult && x > clipmaxx) {
		x = maxx;
	}
#endif
	if(x++ == maxx) {
		x = minx;
		y += yoff;
		height--;
		bitcount = 0;
		imagebits += extra;
	}
  }
  GdFixCursor();
}

/*
 * Draw an ellipse using the current clipping region and foreground color.
 * This just draws in the outline of the ellipse.
 */
void
GdEllipse(PSD psd, COORD x, COORD y, COORD rx, COORD ry)
{
  int xp, yp;			/* current point (based on center) */
  long Asquared;		/* square of x semi axis */
  long TwoAsquared;
  long Bsquared;		/* square of y semi axis */
  long TwoBsquared;
  long d;
  long dx, dy;

  if ((rx < 0) || (ry < 0)) return;

  /* See if the ellipse is either totally visible or totally invisible.
   * If so, then the ellipse drawing is easy.
   */
  switch (GdClipArea(x - rx, y - ry, x + rx, y + ry)) {
      case CLIP_VISIBLE:
	/*
	 * For size considerations, there's no low-level ellipse
	 * draw, so we've got to draw all ellipses
	 * with per-point clipping for the time being
	psd->DrawEllipse(psd, x, y, rx, ry, gr_foreground);
	GdFixCursor();
	return;
	 */
	break;

      case CLIP_INVISIBLE:
	return;
  }

  /*
   * Draw ellipse with per-point clipping
   */
  xp = 0;
  yp = ry;
  Asquared = rx * rx;
  TwoAsquared = 2 * Asquared;
  Bsquared = ry * ry;
  TwoBsquared = 2 * Bsquared;
  d = Bsquared - Asquared * ry + (Asquared >> 2);
  dx = 0;
  dy = TwoAsquared * ry;

  while (dx < dy) {
	draw4points(psd, x, y, xp, yp);
	if (d > 0) {
		yp--;
		dy -= TwoAsquared;
		d -= dy;
	}
	xp++;
	dx += TwoBsquared;
	d += (Bsquared + dx);
  }
  d += ((3L * (Asquared - Bsquared) / 2L - (dx + dy)) >> 1);
  while (yp >= 0) {
	draw4points(psd, x, y, xp, yp);
	if (d < 0) {
		xp++;
		dx += TwoBsquared;
		d += dx;
	}
	yp--;
	dy -= TwoAsquared;
	d += (Asquared - dy);
  }
  GdFixCursor();
}

/* Set four points symmetrically situated around a point. */
static void
draw4points(PSD psd, COORD x,COORD y,COORD px,COORD py)
{
	drawpoint(psd, x + px, y + py);
	drawpoint(psd, x - px, y + py);
	drawpoint(psd, x + px, y - py);
	drawpoint(psd, x - px, y - py);
}

/*
 * Fill an ellipse using the current clipping region and foreground color.
 */
void
GdFillEllipse(PSD psd, COORD x, COORD y, COORD rx, COORD ry)
{
  int xp, yp;			/* current point (based on center) */
  long Asquared;		/* square of x semi axis */
  long TwoAsquared;
  long Bsquared;		/* square of y semi axis */
  long TwoBsquared;
  long d;
  long dx, dy;

  if ((rx < 0) || (ry < 0)) return;

  /* See if the ellipse is either totally visible or totally invisible.
   * If so, then the ellipse filling is easy.
   */
  switch (GdClipArea(x - rx, y - ry, x + rx, y + ry)) {
      case CLIP_VISIBLE:
	/*
	 * For size considerations, there's no low-level ellipse
	 * fill, so we've got to fill all ellipses
	 * with per-point clipping for the time being
	psd->FillEllipse(psd, x, y, rx, ry, gr_foreground);
	GdFixCursor();
	return;
	 */
	break;

      case CLIP_INVISIBLE:
	return;
  }

  /*
   * Fill ellipse with per-point clipping
   */
  xp = 0;
  yp = ry;
  Asquared = rx * rx;
  TwoAsquared = 2 * Asquared;
  Bsquared = ry * ry;
  TwoBsquared = 2 * Bsquared;
  d = Bsquared - Asquared * ry + (Asquared >> 2);
  dx = 0;
  dy = TwoAsquared * ry;

  while (dx < dy) {
	drawrow(psd, x - xp, x + xp, y - yp);
	drawrow(psd, x - xp, x + xp, y + yp);
	if (d > 0) {
		yp--;
		dy -= TwoAsquared;
		d -= dy;
	}
	xp++;
	dx += TwoBsquared;
	d += (Bsquared + dx);
  }
  d += ((3L * (Asquared - Bsquared) / 2L - (dx + dy)) >> 1);
  while (yp >= 0) {
	drawrow(psd, x - xp, x + xp, y - yp);
	drawrow(psd, x - xp, x + xp, y + yp);
	if (d < 0) {
		xp++;
		dx += TwoBsquared;
		d += dx;
	}
	yp--;
	dy -= TwoAsquared;
	d += (Asquared - dy);
  }
  GdFixCursor();
}

/* Draw a polygon in the foreground color, applying clipping if necessary.
 * The polygon is only closed if the first point is repeated at the end.
 * Some care is taken to plot the endpoints correctly if the current
 * drawing mode is XOR.  However, internal crossings are not handled
 * correctly.
 */
void
GdPoly(PSD psd, int count, XYPOINT *points)
{
  COORD firstx;
  COORD firsty;
  BOOL didline;

  if (count < 2)
	  return;
  firstx = points->x;
  firsty = points->y;
  didline = FALSE;

  while (count-- > 1) {
	if (didline && (gr_mode == MODE_XOR))
		drawpoint(psd, points->x, points->y);
	/* note: change to drawline*/
	GdLine(psd, points[0].x, points[0].y, points[1].x, points[1].y, TRUE);
	points++;
	didline = TRUE;
  }
  if (gr_mode != MODE_XOR)
	  return;
  points--;
  if ((points->x == firstx) && (points->y == firsty))
	drawpoint(psd, points->x, points->y);
  GdFixCursor();
}

/*
 * Fill a polygon in the foreground color, applying clipping if necessary.
 * The last point may be a duplicate of the first point, but this is
 * not required.
 * Note: this routine currently only correctly fills convex polygons.
 */
void
GdFillPoly(PSD psd, int count, XYPOINT *points)
{
  XYPOINT *pp;		/* current point */
  COORD miny;		/* minimum row */
  COORD maxy;		/* maximum row */
  COORD minx;		/* minimum column */
  COORD maxx;		/* maximum column */
  int i;		/* counter */

  if (count <= 0)
	  return;

  /* First determine the minimum and maximum rows for the polygon. */
  pp = points;
  miny = pp->y;
  maxy = pp->y;
  for (i = count; i-- > 0; pp++) {
	if (miny > pp->y) miny = pp->y;
	if (maxy < pp->y) maxy = pp->y;
  }
  if (miny < 0)
	  miny = 0;
  if (maxy >= psd->yres)
	  maxy = psd->yres - 1;
  if (miny > maxy)
	  return;

  /* Now for each row, scan the list of points and determine the
   * minimum and maximum x coordinate for each line, and plot the row.
   * The last point connects with the first point automatically.
   */
  for (; miny <= maxy; miny++) {
	minx = COORD_MAX;
	maxx = COORD_MIN;
	pp = points;
	for (i = count; --i > 0; pp++)
		extendrow(miny, pp[0].x, pp[0].y, pp[1].x, pp[1].y,
			&minx, &maxx);
	extendrow(miny, pp[0].x, pp[0].y, points[0].x, points[0].y,
		&minx, &maxx);

	if (minx <= maxx)
		drawrow(psd, minx, maxx, miny);
  }
  GdFixCursor();
}

/* Utility routine for filling polygons.  Find the intersection point (if
 * any) of a horizontal line with an arbitrary line, and extend the current
 * minimum and maximum x values as needed to include the intersection point.
 * Input parms:
 *	y 	row to check for intersection
 *	x1, y1	first endpoint
 *	x2, y2	second enpoint
 *	minxptr	address of current minimum x
 *	maxxptr	address of current maximum x
 */
static void
extendrow(COORD y,COORD x1,COORD y1,COORD x2,COORD y2,COORD *minxptr,
	COORD *maxxptr)
{
  COORD x;			/* x coordinate of intersection */
typedef long NUM;
  NUM num;			/* numerator of fraction */

  /* First make sure the specified line segment includes the specified
   * row number.  If not, then there is no intersection.
   */
  if (((y < y1) || (y > y2)) && ((y < y2) || (y > y1)))
	return;

  /* If a horizontal line, then check the two endpoints. */
  if (y1 == y2) {
	if (*minxptr > x1) *minxptr = x1;
	if (*minxptr > x2) *minxptr = x2;
	if (*maxxptr < x1) *maxxptr = x1;
	if (*maxxptr < x2) *maxxptr = x2;
	return;
  }

  /* If a vertical line, then check the x coordinate. */
  if (x1 == x2) {
	if (*minxptr > x1) *minxptr = x1;
	if (*maxxptr < x1) *maxxptr = x1;
	return;
  }

  /* An arbitrary line.  Calculate the intersection point using the
   * formula x = x1 + (y - y1) * (x2 - x1) / (y2 - y1).
   */
  num = ((NUM) (y - y1)) * (x2 - x1);
  x = x1 + num / (y2 - y1);
  if (*minxptr > x) *minxptr = x;
  if (*maxxptr < x) *maxxptr = x;
}

/*
 * Read a rectangular area of the screen.  
 * The color table is indexed row by row.
 */
void
GdReadArea(PSD psd, COORD x, COORD y, COORD width, COORD height,
	PIXELVAL *pixels)
{
	COORD 		row;
	COORD 		col;

	if (width <= 0 || height <= 0)
		return;

	GdCheckCursor(x, y, x+width-1, y+height-1);
	for (row = 0; row < height; row++)
		for (col = 0; col < width; col++)
			*pixels++ = psd->ReadPixel(psd, x + col, y + row);

	GdFixCursor();
}

/* Draw a rectangle of color values, clipping if necessary.
 * If a color matches the background color,
 * that that pixel is only drawn if the gr_usebg flag is set.
 */
void
GdArea(PSD psd, COORD x, COORD y, COORD width, COORD height, PIXELVAL *pixels)
{
  long cellstodo;		/* remaining number of cells */
  long count;			/* number of cells of same color */
  long cc;			/* current cell count */
  long rows;			/* number of complete rows */
  COORD minx;			/* minimum x value */
  COORD maxx;			/* maximum x value */
  PIXELVAL savecolor;		/* saved foreground color */
  BOOL dodraw;			/* TRUE if draw these points */

  minx = x;
  maxx = x + width - 1;

  /* See if the area is either totally visible or totally invisible. If
   * so, then the area drawing is easy.
   */
  switch (GdClipArea(minx, y, maxx, y + height - 1)) {
      case CLIP_VISIBLE:
	/*
	 * For size considerations, there's no low-level area
	 * draw, so we've got to draw everything with per-point
	 * clipping for the time being
	psd->DrawArea(psd, x, y, width, height, pixels);
	GdFixCursor();
	return;
	 */
	break;

      case CLIP_INVISIBLE:
	return;
  }

  savecolor = gr_foreground;
  cellstodo = width * height;
  while (cellstodo > 0) {
	/* See how many of the adjacent remaining points have the
	 * same color as the next point.
	 */
	gr_foreground = *pixels++;
	dodraw = (gr_usebg || (gr_foreground != gr_background));
	count = 1;
	cellstodo--;
	while ((cellstodo > 0) && (gr_foreground == *pixels)) {
		pixels++;
		count++;
		cellstodo--;
	}

	/* If there is only one point with this color, then draw it
	 * by itself.
	 */
	if (count == 1) {
		if (dodraw)
			drawpoint(psd, x, y);
		if (++x > maxx) {
			x = minx;
			y++;
		}
		continue;
	}

	/* There are multiple points with the same color. If we are
	 * not at the start of a row of the rectangle, then draw this
	 * first row specially.
	 */
	if (x != minx) {
		cc = count;
		if (x + cc - 1 > maxx)
			cc = maxx - x + 1;
		if (dodraw)
			drawrow(psd, x, x + cc - 1, y);
		count -= cc;
		x += cc;
		if (x > maxx) {
			x = minx;
			y++;
		}
	}

	/* Now the x value is at the beginning of a row if there are
	 * any points left to be drawn.  Draw all the complete rows
	 * with one call.
	 */
	rows = count / width;
	if (rows > 0) {
		if (dodraw) {
			/* note: change to fillrect, (parm types changed)*/
			/*GdFillRect(psd, x, y, maxx, y + rows - 1);*/
			GdFillRect(psd, x, y, maxx - x + 1, rows);
		}
		count %= width;
		y += rows;
	}

	/* If there is a final partial row of pixels left to be
	 * drawn, then do that.
	 */
	if (count > 0) {
		if (dodraw)
			drawrow(psd, x, x + count - 1, y);
		x += count;
	}
  }
  gr_foreground = savecolor;
  GdFixCursor();
}

#if NOTYET
/* Copy a rectangular area from one screen area to another.
 * This bypasses clipping.
 */
void
GdCopyArea(PSD psd, COORD srcx, COORD srcy, COORD width, COORD height,
	COORD destx, COORD desty)
{
	if (width <= 0 || height <= 0)
		return;

	if (srcx == destx && srcy == desty)
		return;
	GdCheckCursor(srcx, srcy, srcx + width - 1, srcy + height - 1);
	GdCheckCursor(destx, desty, destx + width - 1, desty + height - 1);
	psd->CopyArea(psd, srcx, srcy, width, height, destx, desty);
	GdFixCursor();
}
#endif

/* Copy source rectangle of pixels to destination rectangle quickly*/
void
GdBlit(PSD dstpsd, COORD dstx, COORD dsty, COORD width, COORD height,
	PSD srcpsd, COORD srcx, COORD srcy, int rop)
{
	int	DSTX, DSTY, WIDTH, HEIGHT;
	int	SRCX, SRCY;

#define REGIONS 0
#if REGIONS
	RECT *		prc;
	extern CLIPREGION *clipregion;
#else
	CLIPRECT *	prc;
	extern CLIPRECT cliprects[];
	extern int clipcount;
#endif
	int		count, dx, dy;

	/*FIXME: compare bpp's and convert if necessary*/
	assert(dstpsd->planes == srcpsd->planes);
	assert(dstpsd->bpp == srcpsd->bpp);
	
	/* clip blit rectangle to source screen/bitmap size*/
	/* we must do this because there isn't any source clipping setup*/
	if(srcx+width > srcpsd->xres)
		width = srcpsd->xres - srcx;
	if(srcy+height > srcpsd->yres)
		height = srcpsd->yres - srcy;

	switch(GdClipArea(dstx, dsty, dstx+width-1, dsty+height-1)) {
	case CLIP_VISIBLE:
		dstpsd->Blit(dstpsd, dstx, dsty, width, height,
			srcpsd, srcx, srcy, rop);
		GdFixCursor();
		return;

	case CLIP_INVISIBLE:
		return;
	}

	/* Partly clipped, we'll blit using destination clip
	 * rectangles, and offset the blit accordingly.
	 * Since the destination is already clipped, we
	 * only need to clip the source here.
	 */
#if REGIONS
	prc = clipregion->rects;
	count = clipregion->numRects;
#else
	prc = cliprects;
	count = clipcount;
#endif
	while(--count >= 0) {
#if REGIONS
		dx = prc->left - dstx;
		dy = prc->top - dsty;
		WIDTH = prc->right - prc->left;
		HEIGHT = prc->bottom - prc->top;
#else
		dx = prc->x - dstx;
		dy = prc->y - dsty;
		WIDTH = prc->width;
		HEIGHT = prc->height;
#endif
		/* calc offset into blit*/
		DSTX = dstx + dx;
		SRCX = srcx + dx;
		DSTY = dsty + dy;
		SRCY = srcy + dy;

		/* clip source rectangle*/
		if(SRCX + WIDTH > srcpsd->xres)
			WIDTH = srcpsd->yres - SRCY;
		if(SRCY + HEIGHT > srcpsd->yres)
			HEIGHT = srcpsd->yres - SRCY;

		GdCheckCursor(DSTX, DSTY, DSTX+WIDTH-1, DSTY+HEIGHT-1);
		dstpsd->Blit(dstpsd, DSTX, DSTY, WIDTH, HEIGHT,
				srcpsd, SRCX, SRCY, rop);
		++prc;
	}
	GdFixCursor();
}
