/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 * 
 * MicroWindows Proportional Font Routine Header (proportional font format)
 *
 * These routines are screen driver entry points.
 */

#define NUMBER_FONTS	4	/* number of compiled-in fonts*/

/* entry points*/
BOOL 	gen_getfontinfo(PSD psd,FONTID fontid,PFONTINFO pfontinfo);
void 	gen_gettextsize(PSD psd,const UCHAR *str,int cc,COORD *retwd,
		COORD *retht,FONTID fontid);
void 	gen_gettextbits(PSD psd,UCHAR ch,IMAGEBITS *retmap,COORD *retwd,
		COORD *retht,FONTID fontid);

/* local data*/
extern PFONT fonts[NUMBER_FONTS];

/* the following aren't used yet*/
void 	gen_drawtext(PSD psd,COORD x,COORD y,const UCHAR *s,int n,PIXELVAL fg,
		FONTID fontid);
void 	gen_drawbitmap(PSD psd,COORD x,COORD y,COORD width,COORD height,
		IMAGEBITS *table, PIXELVAL fgcolor);
