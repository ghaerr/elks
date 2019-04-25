/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 4bpp (16 color) standard palette definition
 */
#include "device.h"

/*
 * Standard palette for 16 color systems.
 */
RGBENTRY stdpal4[16] = {
	/* 16 EGA colors, arranged in VGA standard palette order*/
	RGBDEF( 0  , 0  , 0   ),	/* black*/
	RGBDEF( 0  , 0  , 128 ),	/* blue*/
	RGBDEF( 0  , 128, 0   ),	/* green*/
	RGBDEF( 0  , 128, 128 ),	/* cyan*/
	RGBDEF( 128, 0  , 0   ),	/* red*/
	RGBDEF( 128, 0  , 128 ),	/* magenta*/
	RGBDEF( 128, 64 , 0   ),	/* adjusted brown*/
	RGBDEF( 192, 192, 192 ),	/* ltgray*/
	RGBDEF( 128, 128, 128 ),	/* gray*/
	RGBDEF( 0  , 0  , 255 ),	/* ltblue*/
	RGBDEF( 0  , 255, 0   ),	/* ltgreen*/
	RGBDEF( 0  , 255, 255 ),	/* ltcyan*/
	RGBDEF( 255, 0  , 0   ),	/* ltred*/
	RGBDEF( 255, 0  , 255 ),	/* ltmagenta*/
	RGBDEF( 255, 255, 0   ),	/* yellow*/
	RGBDEF( 255, 255, 255 ),	/* white*/
};
