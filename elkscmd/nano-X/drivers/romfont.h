/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 *
 * PC ROM Font Routine Header (PC ROM font format)
 *
 * These routines are screen driver entry points.
 */

/* compiled in fonts*/
#define NUMBER_FONTS	1	/* rom font only for now*/

#define	ROM_CHAR_WIDTH	8	/* number of pixels for character width */
#define MAX_ROM_HEIGHT	16	/* max rom character height*/
#define	FONT_CHARS	256	/* number of characters in font tables */

/* int10 functions*/
#define FNGETROMADDR	0x1130	/* function for address of rom character table*/
#define GETROM8x14	0x0200	/* want address of ROM 8x14 char table*/

/* entry points*/
void	pcrom_init(PSD psd);
BOOL 	pcrom_getfontinfo(PSD psd,FONTID fontid,PFONTINFO pfontinfo);
void 	pcrom_gettextsize(PSD psd,const UCHAR *str,int cc,COORD *retwd,
		COORD *retht,FONTID fontid);
void 	pcrom_gettextbits(PSD psd,UCHAR ch,IMAGEBITS *retmap,COORD *retwd,
		COORD *retht,FONTID fontid);

/* local data*/
extern int	ROM_CHAR_HEIGHT; /* number of scan lines in fonts in ROM */
extern FARADDR 	rom_char_addr;

/* the following aren't used yet*/
void 	pcrom_drawtext(PSD psd,COORD x,COORD y,const UCHAR *s,int n,PIXELVAL fg,
		FONTID fontid);
void 	gen_drawbitmap(PSD psd,COORD x,COORD y,COORD width,COORD height,
		IMAGEBITS *table, PIXELVAL fgcolor);
