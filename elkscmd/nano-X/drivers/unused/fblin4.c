/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 4bpp Packed Linear Video Driver for MicroWindows
 * 	This driver is written for the Vr41xx Palm PC machines
 * 	Hopefully, we can get the 4bpp mode running 320x240x16
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "../device.h"
#include "fb.h"

static unsigned char notmask[2] = { 0x0f, 0xf0};

/* Calc linelen and mmap size, return 0 on fail*/
int
linear4_init(PSD psd)
{
	psd->size = psd->yres * psd->linelen;
	/* linelen in bytes for bpp 1, 2, 4, 8 so no change*/
	return 1;
}


/* Set pixel at x, y, to pixelval c*/
void
linear4_drawpixel(PSD psd, COORD x, COORD y, PIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c >= 0 && c < psd->ncolors);

	++drawing;
	addr += x/2 + y * psd->linelen;
	if(gr_mode == MODE_XOR)
		*addr = (*addr & notmask[x&1]) ^ (c << ((1-(x&1))*4));
	else
		*addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))*4));
	--drawing;
}

/* Read pixel at x, y*/
PIXELVAL
linear4_readpixel(PSD psd, COORD x, COORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return (addr[x/2 + y * psd->linelen] >> ((1-(x&1))*4) ) & 0x0f;
}

/* Draw horizontal line from x1,y to x2,y not including final point*/
void
linear4_drawhorzline(PSD psd, COORD x1, COORD x2, COORD y, PIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 <= psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c >= 0 && c < psd->ncolors);

	++drawing;
	addr += x1/2 + y * psd->linelen;
	if(gr_mode == MODE_XOR) {
		while(x1 < x2) {
			*addr = (*addr & notmask[x1&1]) ^ (c << ((1-(x1&1))*4));
			if((++x1 & 1) == 0)
				++addr;
		}
	} else {
		while(x1 < x2) {
			*addr = (*addr & notmask[x1&1]) | (c << ((1-(x1&1))*4));
			if((++x1 & 1) == 0)
				++addr;
		}
	}
	--drawing;
}

/* Draw a vertical line from x,y1 to x,y2 not including final point*/
void
linear4_drawvertline(PSD psd, COORD x, COORD y1, COORD y2, PIXELVAL c)
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
	addr += x/2 + y1 * linelen;
	if(gr_mode == MODE_XOR)
		while(y1++ < y2) {
			*addr = (*addr & notmask[x&1]) ^ (c << ((1-(x&1))*4));
			addr += linelen;
		}
	else
		while(y1++ < y2) {
			*addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))*4));
			addr += linelen;
		}
	--drawing;
}

/* srccopy bitblt, opcode is currently ignored*/
void
linear4_blit(PSD dstpsd, COORD dstx, COORD dsty, COORD w, COORD h,
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
	dst = dstpsd->addr + dstx/2 + dsty * dlinelen;
	src = srcpsd->addr + srcx/2 + srcy * slinelen;
	while(--h >= 0) {
		ADDR8	d = dst;
		ADDR8	s = src;
		COORD	dx = dstx;
		COORD	sx = srcx;
		for(i=0; i<w; ++i) {
			*d = (*d & notmask[dx&1]) |
			   ((*s >> ((1-(sx&1))*4) & 0x0f) << ((1-(dx&1))*4));
			if((++dx & 1) == 0)
				++d;
			if((++sx & 1) == 0)
				++s;
		}
		dst += dlinelen;
		src += slinelen;
	}
	--drawing;
}

FBENTRY fblinear4 = {
	linear4_init,
	linear4_drawpixel,
	linear4_readpixel,
	linear4_drawhorzline,
	linear4_drawvertline,
	linear4_blit
};
