/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Null Linear Video Driver for MicroWindows
 *
 * Inspired from Ben Pfaff's BOGL <pfaffben@debian.org>
 */
#include "../device.h"
#include "fb.h"

static int
null_init(PSD psd)
{
	return 0;
}

static void
null_drawpixel(PSD psd, COORD x, COORD y, PIXELVAL c)
{
}

static PIXELVAL
null_readpixel(PSD psd, COORD x, COORD y)
{
	return 0;
}

static void 	
null_drawhorzline(PSD psd, COORD x1, COORD x2, COORD y, PIXELVAL c)
{
}

static void	
null_drawvertline(PSD psd, COORD x, COORD y1, COORD y2, PIXELVAL c)
{
}

static void	
null_blit(PSD destpsd, COORD destx, COORD desty, COORD w, COORD h,
	PSD srcpsd, COORD srcx, COORD srcy, int op)
{
}

FBENTRY fbnull = {
	null_init,
	null_drawpixel,
	null_readpixel,
	null_drawhorzline,
	null_drawvertline,
	null_blit
};
