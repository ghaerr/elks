/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 1bpp (2 color) standard palette definition
 */
#include "device.h"

/*
 * Standard palette for 2 color (monochrome) systems.
 */
RGBENTRY stdpal1[2] = {
	RGBDEF( 0  , 0  , 0   ),	/* black*/
	RGBDEF( 255, 255, 255 )		/* white*/
};
