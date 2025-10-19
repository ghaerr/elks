/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 2bpp Packed Linear Video Driver for MicroWindows
 * 	This driver is written for the Vr41xx Palm PC machines
 * 	The screen is 320x240x4
 *
 * 	In this driver, psd->linelen is line byte length, not line pixel length
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "../device.h"
#include "fb.h"

static unsigned char notmask[4] = { 0x3f, 0xcf, 0xf3, 0xfc};

/* Calc linelen and mmap size, return 0 on fail*/
int
linear2_init(PSD psd)
{
	psd->size = psd->yres * psd->linelen;
	/* linelen in bytes for bpp 1, 2, 4, 8 so no change*/
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
void
linear2_drawpixel(PSD psd, COORD x, COORD y, PIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c >= 0 && c < psd->ncolors);

	++drawing;
	addr += x/4 + y * psd->linelen;
	if(gr_mode == MODE_XOR)
		*addr = (*addr & notmask[x&3]) ^ (c << ((3-(x&3))*2));
	else
		*addr = (*addr & notmask[x&3]) | (c << ((3-(x&3))*2));
	--drawing;
}

/* Read pixel at x, y*/
PIXELVAL
linear2_readpixel(PSD psd, COORD x, COORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return (addr[x/4 + y * psd->linelen] >> ((3-(x&3))*2) ) & 0x03;
}

/* Draw horizontal line from x1,y to x2,y not including final point*/
void
linear2_drawhorzline(PSD psd, COORD x1, COORD x2, COORD y, PIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 <= psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c >= 0 && c < psd->ncolors);

	++drawing;
	addr += x1/4 + y * psd->linelen;
	if(gr_mode == MODE_XOR) {
		while(x1 < x2) {
			*addr = (*addr & notmask[x1&3]) ^ (c << ((3-(x1&3))*2));
			if((++x1 & 3) == 0)
				++addr;
		}
	} else {
		while(x1 < x2) {
			*addr = (*addr & notmask[x1&3]) | (c << ((3-(x1&3))*2));
			if((++x1 & 3) == 0)
				++addr;
		}
	}
	--drawing;
}

/* Draw a vertical line from x,y1 to x,y2 not including final point*/
void
linear2_drawvertline(PSD psd, COORD x, COORD y1, COORD y2, PIXELVAL c)
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
	addr += x/4 + y1 * linelen;
	if(gr_mode == MODE_XOR)
		while(y1++ < y2) {
			*addr = (*addr & notmask[x&3]) ^ (c << ((3-(x&3))*2));
			addr += linelen;
		}
	else
		while(y1++ < y2) {
			*addr = (*addr & notmask[x&3]) | (c << ((3-(x&3))*2));
			addr += linelen;
		}
	--drawing;
}

/* srccopy bitblt, opcode is currently ignored*/
void
linear2_blit(PSD dstpsd, COORD dstx, COORD dsty, COORD w, COORD h,
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
	dst = dstpsd->addr + dstx/4 + dsty * dlinelen;
	src = srcpsd->addr + srcx/4 + srcy * slinelen;
	while(--h >= 0) {
		ADDR8	d = dst;
		ADDR8	s = src;
		COORD	dx = dstx;
		COORD	sx = srcx;
		for(i=0; i<w; ++i) {
			*d = (*d & notmask[dx&3]) |
			   ((*s >> ((3-(sx&3))*2) & 0x03) << ((3-(dx&3))*2));
			if((++dx & 3) == 0)
				++d;
			if((++sx & 3) == 0)
				++s;
		}
		dst += dlinelen;
		src += slinelen;
	}
	--drawing;
}

FBENTRY fblinear2 = {
	linear2_init,
	linear2_drawpixel,
	linear2_readpixel,
	linear2_drawhorzline,
	linear2_drawvertline,
	linear2_blit
};
