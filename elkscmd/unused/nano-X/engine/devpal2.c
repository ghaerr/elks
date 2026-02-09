/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 2pbb (4 color) standard palette definition
 */
#include "device.h"

/*
 * Standard palette for Everex Freestyle Palm PC
 * This palette is in reverse order from some 2bpp systems.
 * That is, white is pixel value 0, while black is 3.
 */
RGBENTRY stdpal2[4] = {
	RGBDEF( 255, 255, 255 ),	/* white*/
	RGBDEF( 192, 192, 192 ),	/* ltgray*/
	RGBDEF( 128, 128, 128 ),	/* gray*/
	RGBDEF( 0  , 0  , 0   )		/* black*/
};
