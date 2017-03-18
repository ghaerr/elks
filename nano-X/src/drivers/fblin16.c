/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 16bpp Linear Video Driver for MicroWindows
 *
 * Inspired from Ben Pfaff's BOGL <pfaffben@debian.org>
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include <wchar.h>
#include "../device.h"
#include "fb.h"

/* Calc linelen and mmap size, return 0 on fail*/
int
linear16_init(PSD psd)
{
	psd->size = psd->yres * psd->linelen;
	/* convert linelen from byte to pixel length for bpp 16, 32*/
	psd->linelen /= 2;
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
void
linear16_drawpixel(PSD psd, COORD x, COORD y, PIXELVAL c)
{
	ADDR16	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c >= 0 && c < psd->ncolors);

	++drawing;
	if(gr_mode == MODE_XOR)
		addr[x + y * psd->linelen] ^= c;
	else
		addr[x + y * psd->linelen] = c;
	--drawing;
}

/* Read pixel at x, y*/
PIXELVAL
linear16_readpixel(PSD psd, COORD x, COORD y)
{
	ADDR16	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return addr[x + y * psd->linelen];
}

/* Draw horizontal line from x1,y to x2,y not including final point*/
void
linear16_drawhorzline(PSD psd, COORD x1, COORD x2, COORD y, PIXELVAL c)
{
	ADDR16	addr = psd->addr;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 <= psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c >= 0 && c < psd->ncolors);

	++drawing;
	addr += x1 + y * psd->linelen;
	if(gr_mode == MODE_XOR) {
		while(x1++ < x2)
			*addr++ ^= c;
	} else
		//FIXME: memsetw(dst, c, x2-x1)?
		while(x1++ < x2)
			*addr++ = c;
	--drawing;
}

/* Draw a vertical line from x,y1 to x,y2 not including final point*/
void
linear16_drawvertline(PSD psd, COORD x, COORD y1, COORD y2, PIXELVAL c)
{
	ADDR16	addr = psd->addr;
	int	linelen = psd->linelen;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 <= psd->yres);
	assert (y2 >= y1);
	assert (c >= 0 && c < psd->ncolors);

	++drawing;
	addr += x + y1 * linelen;
	if(gr_mode == MODE_XOR)
		while(y1++ < y2) {
			*addr ^= c;
			addr += linelen;
		}
	else
		while(y1++ < y2) {
			*addr = c;
			addr += linelen;
		}
	--drawing;
}

/* srccopy bitblt, opcode is currently ignored*/
void
linear16_blit(PSD dstpsd, COORD dstx, COORD dsty, COORD w, COORD h,
	PSD srcpsd, COORD srcx, COORD srcy, int op)
{
	ADDR16	dst;
	ADDR16	src;
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
	dst = dstpsd->addr + dstx + dsty * dlinelen;
	src = srcpsd->addr + srcx + srcy * slinelen;
	while(--h >= 0) {
#if 0
		/* a _fast_ memcpy is a _must_ in this routine*/
		wmemcpy(dst, src, w);
		dst += dlinelen;
		src += slinelen;
#else
		for(i=0; i<w; ++i)
			*dst++ = *src++;
		dst += dlinelen - w;
		src += slinelen - w;
#endif
	}
	--drawing;
}

FBENTRY fblinear16 = {
	linear16_init,
	linear16_drawpixel,
	linear16_readpixel,
	linear16_drawhorzline,
	linear16_drawvertline,
	linear16_blit
};
