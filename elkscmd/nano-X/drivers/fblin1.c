/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 1bpp Packed Linear Video Driver for MicroWindows
 *
 * 	In this driver, psd->linelen is line byte length, not line pixel length
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "../device.h"
#include "fb.h"

static unsigned char notmask[8] = {
	0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe};

/* Calc linelen and mmap size, return 0 on fail*/
int
linear1_init(PSD psd)
{
	psd->size = psd->yres * psd->linelen;
	/* linelen in bytes for bpp 1, 2, 4, 8 so no change*/
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
void
linear1_drawpixel(PSD psd, COORD x, COORD y, PIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c >= 0 && c < psd->ncolors);

	++drawing;
	addr += x/8 + y * psd->linelen;
	if (gr_mode == MODE_XOR)
		*addr = (*addr & notmask[x&7]) ^ (c << (7-(x&7)));
	else
		*addr = (*addr & notmask[x&7]) | (c << (7-(x&7)));
	--drawing;
}

/* Read pixel at x, y*/
PIXELVAL
linear1_readpixel(PSD psd, COORD x, COORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return (addr[x/8 + y * psd->linelen] >> (7-(x&7)) ) & 0x01;
}

/* Draw horizontal line from x1,y to x2,y not including final point*/
void
linear1_drawhorzline(PSD psd, COORD x1, COORD x2, COORD y, PIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 <= psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c >= 0 && c < psd->ncolors);

	++drawing;
	addr += x1/8 + y * psd->linelen;
	if (gr_mode == MODE_XOR) {
		while (x1 < x2) {
			*addr = (*addr & notmask[x1&7]) ^ (c << (7-(x1&7)));
			if ((++x1 & 7) == 0)
				++addr;
		}
	} else {
		while (x1 < x2) {
			*addr = (*addr & notmask[x1&7]) | (c << (7-(x1&7)));
			if ((++x1 & 7) == 0)
				++addr;
		}
	}
	--drawing;
}

/* Draw a vertical line from x,y1 to x,y2 not including final point*/
void
linear1_drawvertline(PSD psd, COORD x, COORD y1, COORD y2, PIXELVAL c)
{
	ADDR8	addr = psd->addr;
	int	linelen = psd->linelen;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 <= psd->yres);
	assert (y2 >= y1);
	assert (c >= 0 && c < psd->ncolors);

	++drawing;
	addr += x/8 + y1 * linelen;
	if (gr_mode == MODE_XOR)
		while (y1++ < y2) {
			*addr = (*addr & notmask[x&7]) ^ (c << (7-(x&7)));
			addr += linelen;
		}
	else
		while (y1++ < y2) {
			*addr = (*addr & notmask[x&7]) | (c << (7-(x&7)));
			addr += linelen;
		}
	--drawing;
}

/* srccopy bitblt, opcode is currently ignored*/
void
linear1_blit(PSD dstpsd, COORD dstx, COORD dsty, COORD w, COORD h,
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

	++drawing;
	dst = dstpsd->addr + dstx/8 + dsty * dlinelen;
	src = srcpsd->addr + srcx/8 + srcy * slinelen;
	while (--h >= 0) {
		ADDR8	d = dst;
		ADDR8	s = src;
		COORD	dx = dstx;
		COORD	sx = srcx;
		for (i=0; i<w; ++i) {
			*d = (*d & notmask[dx&7]) |
			   ((*s >> (7-(sx&7)) & 0x01) << (7-(dx&7)));
			if ((++dx & 7) == 0)
				++d;
			if ((++sx & 7) == 0)
				++s;
		}
		dst += dlinelen;
		src += slinelen;
	}
	--drawing;
}

FBENTRY fblinear1 = {
	linear1_init,
	linear1_drawpixel,
	linear1_readpixel,
	linear1_drawhorzline,
	linear1_drawvertline,
	linear1_blit
};
