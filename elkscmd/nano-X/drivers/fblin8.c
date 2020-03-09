/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 8bpp Linear Video Driver for MicroWindows
 *
 * Inspired from Ben Pfaff's BOGL <pfaffben@debian.org>
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "../device.h"
#include "fb.h"

/* Calc linelen and mmap size, return 0 on fail*/
int
linear8_init(PSD psd)
{
	psd->size = psd->yres * psd->linelen;
	/* linelen in bytes for bpp 1, 2, 4, 8 so no change*/
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
void
linear8_drawpixel(PSD psd, COORD x, COORD y, PIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c >= 0 && c < psd->ncolors);

	++drawing;
	if (gr_mode == MODE_XOR)
		addr[x + y * psd->linelen] ^= c;
	else
		addr[x + y * psd->linelen] = c;
	--drawing;
}

/* Read pixel at x, y*/
PIXELVAL
linear8_readpixel(PSD psd, COORD x, COORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return addr[x + y * psd->linelen];
}

/* Draw horizontal line from x1,y to x2,y not including final point*/
void
linear8_drawhorzline(PSD psd, COORD x1, COORD x2, COORD y, PIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 <= psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c >= 0 && c < psd->ncolors);

	++drawing;
	addr += x1 + y * psd->linelen;
	if (gr_mode == MODE_XOR) {
		while (x1++ < x2)
			*addr++ ^= c;
	} else
		memset(addr, c, x2 - x1);
	--drawing;
}

/* Draw a vertical line from x,y1 to x,y2 not including final point*/
void
linear8_drawvertline(PSD psd, COORD x, COORD y1, COORD y2, PIXELVAL c)
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
	addr += x + y1 * linelen;
	if (gr_mode == MODE_XOR)
		while (y1++ < y2) {
			*addr ^= c;
			addr += linelen;
		}
	else
		while (y1++ < y2) {
			*addr = c;
			addr += linelen;
		}
	--drawing;
}

/* srccopy bitblt, opcode is currently ignored*/
void
linear8_blit(PSD dstpsd, COORD dstx, COORD dsty, COORD w, COORD h,
	PSD srcpsd, COORD srcx, COORD srcy, int op)
{
	ADDR8	dst;
	ADDR8	src;
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
	dst = dstpsd->addr + dstx + dsty * dlinelen;
	src = srcpsd->addr + srcx + srcy * slinelen;
	while (--h >= 0) {
		/* a _fast_ memcpy is a _must_ in this routine*/
		memcpy(dst, src, w);
		dst += dlinelen;
		src += slinelen;
	}
	--drawing;
}

FBENTRY fblinear8 = {
	linear8_init,
	linear8_drawpixel,
	linear8_readpixel,
	linear8_drawhorzline,
	linear8_drawvertline,
	linear8_blit
};
