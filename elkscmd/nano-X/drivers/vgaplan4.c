/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 16 color 4 planes EGA/VGA Planar Video Driver for MicroWindows
 * Portable C version
 * Blitting enabled with #define HAVEBLIT in vgaplan4.h
 *
 * Based on BOGL - Ben's Own Graphics Library.
 *   Written by Ben Pfaff <pfaffben@debian.org>.
 *	 BOGL is licensed under the terms of the GNU General Public License
 *
 * In this driver, psd->linelen is line byte length, not line pixel length
 *
 * This file is meant to compile under Linux, ELKS, and MSDOS
 * without changes.  Please try to keep it that way.
 * 
 */
/*#define NDEBUG*/
#include <assert.h>
#if LINUX
#include <sys/io.h>
#endif
#include "../device.h"
#include "vgaplan4.h"
#include "fb.h"

#if MSDOS | ELKS
/* assumptions for speed: NOTE: psd is ignored in these routines*/
#define SCREENBASE 		MK_FP(0xa000, 0)
#define BYTESPERLINE		80
#else
#define SCREENBASE 		((char *)psd->addr)
#define BYTESPERLINE		(psd->linelen)
#endif

/* extern data*/
extern MODE gr_mode;	/* temp kluge*/

/* Values for the data rotate register to implement drawing modes. */
static unsigned char mode_table[MODE_MAX + 1] = {
	0x00, 0x18, 0x10, 0x08
};

/* precalculated mask bits*/
static unsigned char mask[8] = {
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

/* Init VGA controller, calc linelen and mmap size, return 0 on fail*/
int
ega_init(PSD psd)
{
#if LINUX
	/* allow direct access to vga controller space*/
	if(ioperm(0x3c0, 0x20, 1) == -1)
		return 0;
#endif

#if MSDOS | ELKS
	/* fill in screendevice struct if not framebuffer driver*/
	psd->addr = (char *)SCREENBASE;		/* long ptr -> short on 16bit sys*/
	psd->linelen = BYTESPERLINE;
	psd->size = 65535;
#else
	/* framebuffer mmap size*/
	psd->size = 0x10000;
#endif

	/* Set up some default values for the VGA Graphics Registers. */
	set_enable_sr (0x0f);
	set_op (0);
	set_mode (0);

	return 1;
}

/* draw a pixel at x,y of color c*/
void
ega_drawpixel(PSD psd,unsigned int x, unsigned int y, PIXELVAL c)
{
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c >= 0 && c < psd->ncolors);
  
	DRAWON;
	set_op(mode_table[gr_mode]);
	set_color (c);
	select_mask ();
	set_mask (mask[x&7]);
	RMW_FP (SCREENBASE + x / 8 + y * BYTESPERLINE);
	DRAWOFF;
}

/* Return 4-bit pixel value at x,y*/
PIXELVAL
ega_readpixel(PSD psd,unsigned int x,unsigned int y)
{
	FARADDR		src;
	int		plane;
	PIXELVAL	c = 0;
	
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
  
	DRAWON;
	src = SCREENBASE + x / 8 + y * BYTESPERLINE;
	for(plane=0; plane<4; ++plane) {
		set_read_plane(plane);
		if(GETBYTE_FP(src) & mask[x&7])
			c |= 1 << plane;
	}
	DRAWOFF;
	return c;
}

/* Draw horizontal line from x1,y to x2,y not including final point*/
void
ega_drawhorzline(PSD psd, unsigned int x1, unsigned int x2, unsigned int y,
	PIXELVAL c)
{
	FARADDR dst, last;

	--x2;
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c >= 0 && c < psd->ncolors);

	DRAWON;
	set_color (c);
	set_op(mode_table[gr_mode]);
	/*
	* The following fast drawhline code is buggy for XOR drawing,
	* for some reason.  So, we use the equivalent slower drawpixel
	* method when not drawing MODE_SET.
	*/
	if(gr_mode == MODE_SET) {
		dst = SCREENBASE + x1 / 8 + y * BYTESPERLINE;
		select_mask ();
		if (x1 / 8 == x2 / 8) {
			set_mask ((0xff >> (x1 % 8)) & (0xff << (7 - x2 % 8)));
			RMW_FP (dst);
		} else {

			set_mask (0xff >> (x1 % 8));
			RMW_FP (dst++);

			set_mask (0xff);
			last = SCREENBASE + x2 / 8 + y * BYTESPERLINE;
			while (dst < last)
				PUTBYTE_FP(dst++, 1);

			set_mask (0xff << (7 - x2 % 8));
			RMW_FP (dst);
		}
	} else {
		/* slower method, draw pixel by pixel*/
		select_mask ();
		while(x1 < x2) {
			set_mask (mask[x1&7]);
			RMW_FP (SCREENBASE + x1++ / 8 + y * BYTESPERLINE);
		}
	}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 not including final point*/
void
ega_drawvertline(PSD psd,unsigned int x, unsigned int y1, unsigned int y2,
	PIXELVAL c)
{
	FARADDR dst, last;

	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c >= 0 && c < psd->ncolors);

	DRAWON;
	set_op(mode_table[gr_mode]);
	set_color (c);
	select_mask ();
	set_mask (mask[x&7]);
	dst = SCREENBASE + x / 8 + y1 * BYTESPERLINE;
	last = SCREENBASE + x / 8 + y2 * BYTESPERLINE;
	while (dst < last) {
		RMW_FP (dst);
		dst += BYTESPERLINE;
	}
	DRAWOFF;
}

void
ega_blit(PSD dstpsd, COORD dstx, COORD dsty, COORD w, COORD h,
	PSD srcpsd, COORD srcx, COORD srcy, int op)
{
#if HAVEBLIT
	BOOL	srcvga, dstvga;

	/* decide which blit algorithm to use*/
	srcvga = srcpsd->flags & PSF_SCREEN;
	dstvga = dstpsd->flags & PSF_SCREEN;

	if(srcvga) {
		if(dstvga)
			vga_to_vga_blit(dstpsd, dstx, dsty, w, h,
				srcpsd, srcx, srcy, op);
		else
			vga_to_mempl4_blit(dstpsd, dstx, dsty, w, h,
				srcpsd, srcx, srcy, op);
	} else {
		if(dstvga)
			mempl4_to_vga_blit(dstpsd, dstx, dsty, w, h,
				srcpsd, srcx, srcy, op);
		else
			mempl4_to_mempl4_blit(dstpsd, dstx, dsty, w, h,
				srcpsd, srcx, srcy, op);
	}
#endif /* HAVEBLIT*/
}

#if LINUX
FBENTRY fbvgaplan4 = {
	(void *)ega_init,
	(void *)ega_drawpixel,
	(void *)ega_readpixel,
	(void *)ega_drawhorzline,
	(void *)ega_drawvertline,
	(void *)ega_blit
};
#endif
