/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 4 planes EGA/VGA Memory Video Driver for Microwindows
 * Optional driver included with #define HAVEBLIT in vgaplan4.h
 * 
 * 4bpp colors are stored in linear 4pp format in memory dc's
 *
 * 	In this driver, psd->linelen is line byte length, not line pixel length
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "vga_dev.h"
#include "vgaplan4.h"

typedef unsigned char *         ADDR8;

#define DRAWON
#define DRAWOFF

#if HAVEBLIT

#if MSDOS | ELKS | __rtems__
/* assumptions for speed: NOTE: psd is ignored in these routines*/
#define SCREENBASE(psd) 	EGA_BASE
#define BYTESPERLINE(psd)	80
#else
/* run on top of framebuffer*/
#define SCREENBASE(psd) 	((char *)((psd)->addr))
#define BYTESPERLINE(psd)	((psd)->linelen)
#endif

/* extern data*/
extern MODE gr_mode;	/* temp kluge*/

static unsigned char notmask[2] = { 0x0f, 0xf0};

/* Set pixel at x, y, to pixelval c*/
void
mempl4_drawpixel(PSD psd, COORD x, COORD y, PIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	addr += (x>>1) + y * psd->linelen;
	if (gr_mode == MODE_XOR)
		*addr ^= c << ((1-(x&1))*4);
	else
		*addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))*4));
}

/* Read pixel at x, y*/
PIXELVAL
mempl4_readpixel(PSD psd, COORD x, COORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return (addr[(x>>1) + y * psd->linelen] >> ((1-(x&1))*4) ) & 0x0f;
}

/* Draw horizontal line from x1,y to x2,y not including final point*/
void
mempl4_drawhorzline(PSD psd, COORD x1, COORD x2, COORD y, PIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 <= psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	addr += (x1>>1) + y * psd->linelen;
	if (gr_mode == MODE_XOR) {
		while (x1 < x2) {
			*addr ^= c << ((1-(x1&1))*4);
			if ((++x1 & 1) == 0)
				++addr;
		}
	} else {
		while (x1 < x2) {
			*addr = (*addr & notmask[x1&1]) | (c << ((1-(x1&1))*4));
			if ((++x1 & 1) == 0)
				++addr;
		}
	}
}

/* Draw a vertical line from x,y1 to x,y2 not including final point*/
void
mempl4_drawvertline(PSD psd, COORD x, COORD y1, COORD y2, PIXELVAL c)
{
	ADDR8	addr = psd->addr;
	int	linelen = psd->linelen;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 <= psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	addr += (x>>1) + y1 * linelen;
	if (gr_mode == MODE_XOR)
		while (y1++ < y2) {
			*addr ^= c << ((1-(x&1))*4);
			addr += linelen;
		}
	else
		while (y1++ < y2) {
			*addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))*4));
			addr += linelen;
		}
}

/* srccopy bitblt, opcode is currently ignored*/
/* copy from memdc to memdc*/
void
mempl4_to_mempl4_blit(PSD dstpsd, COORD dstx, COORD dsty, COORD w, COORD h,
	PSD srcpsd, COORD srcx, COORD srcy, int op)
{
	ADDR8	dst;
	ADDR8	src;
	int	i;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;

	assert (dstpsd->addr != 0);
	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (w > 0);
	assert (h > 0);
	assert (srcpsd->addr != 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);

	dst = (char *)dstpsd->addr + (dstx>>1) + dsty * dlinelen;
	src = (char *)srcpsd->addr + (srcx>>1) + srcy * slinelen;
	while (--h >= 0) {
		volatile ADDR8	d = dst;
		volatile ADDR8	s = src;
		COORD		dx = dstx;
		COORD		sx = srcx;
		for (i=0; i<w; ++i) {
			*d = (*d & notmask[dx&1]) |
			   ((*s >> ((1-(sx&1))*4) & 0x0f) << ((1-(dx&1))*4));
			if ((++dx & 1) == 0)
				++d;
			if ((++sx & 1) == 0)
				++s;
		}
		dst += dlinelen;
		src += slinelen;
	}
}

/* srccopy bitblt, opcode is currently ignored*/
/* copy from vga memory to vga memory, psd's ignored for speed*/
void
vga_to_vga_blit(PSD dstpsd, COORD dstx, COORD dsty, COORD w, COORD h,
	PSD srcpsd, COORD srcx, COORD srcy, int op)
{
	volatile FARADDR	dst;
	volatile FARADDR	src;
	int	i, plane;
	int	x1, x2;

	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (w > 0);
	assert (h > 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);

	DRAWON;
	set_enable_sr(0);
	dst = SCREENBASE(dstpsd) + (dstx>>3) + dsty * BYTESPERLINE(dstpsd);
	src = SCREENBASE(srcpsd) + (srcx>>3) + srcy * BYTESPERLINE(srcpsd);
	x1 = dstx>>3;
	x2 = (dstx + w - 1) >> 3;
	while (--h >= 0) {
		for (plane=0; plane<4; ++plane) {
			volatile FARADDR d = dst;
			volatile FARADDR s = src;

	    		set_read_plane(plane);
			set_write_planes(1 << plane);

			/* FIXME: only works if srcx and dstx are same modulo*/
			select_mask();
			if (x1 == x2) {
		  		set_mask((0xff >> (x1 & 7)) & (0xff << (7 - (x2 & 7))));
				PUTBYTE_FP(d, GETBYTE_FP(s));
			} else {
				set_mask(0xff >> (x1 & 7));
				PUTBYTE_FP(d++, GETBYTE_FP(s++));

				set_mask(0xff);
		  		for (i=x1+1; i<x2; ++i)
					PUTBYTE_FP(d++, GETBYTE_FP(s++));

		  		set_mask(0xff << (7 - (x2 & 7)));
				PUTBYTE_FP(d, GETBYTE_FP(s));
			}
		}
		dst += BYTESPERLINE(dstpsd);
		src += BYTESPERLINE(srcpsd);
	}
	set_write_planes(0x0f);
	set_enable_sr(0x0f);
	DRAWOFF;
}

/* srccopy bitblt, opcode is currently ignored*/
/* copy from memdc to vga memory*/
void
mempl4_to_vga_blit(PSD dstpsd, COORD dstx, COORD dsty, COORD w, COORD h,
	PSD srcpsd, COORD srcx, COORD srcy, int op)
{
	volatile FARADDR	dst;
	ADDR8	src;
	int	i;
	int	slinelen = srcpsd->linelen;
	static unsigned char mask[8] = {
		0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
	};

	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (w > 0);
	assert (h > 0);
	assert (srcpsd->addr != 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);

	DRAWON;
	set_op(0);		/* modetable[MODE_SET]*/
	dst = SCREENBASE(dstpsd) + (dstx>>3) + dsty * BYTESPERLINE(dstpsd);
	src = (char *)srcpsd->addr + (srcx>>1) + srcy * slinelen;
	while (--h >= 0) {
		volatile FARADDR d = dst;
		volatile ADDR8	s = src;
		COORD		dx = dstx;
		COORD		sx = srcx;
		for (i=0; i<w; ++i) {
			set_color(*s >> ((1-(sx&1))*4) & 0x0f);
			select_mask();
			set_mask (mask[dx&7]);
			RMW_FP(d);

			if ((++dx & 7) == 0)
				++d;
			if ((++sx & 1) == 0)
				++s;
		}
		dst += BYTESPERLINE(dstpsd);
		src += slinelen;
	}
	DRAWOFF;
}

/* srccopy bitblt, opcode is currently ignored*/
/* copy from vga memory to memdc*/
void
vga_to_mempl4_blit(PSD dstpsd, COORD dstx, COORD dsty, COORD w, COORD h,
	PSD srcpsd, COORD srcx, COORD srcy, int op)
{
	/* FIXME*/
}

#endif /* HAVEBLIT*/
