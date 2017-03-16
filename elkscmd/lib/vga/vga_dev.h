#ifndef _DEVICE_H
#define _DEVICE_H
/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Mid-level Screen, Mouse and Keyboard device driver API's
 */

/* Changeable limits */
#define ALPHABLEND	1			/* =1 to include blending code*/
#define	MAX_CLIPRECTS 	200			/* maximum clip rectangles */
#define	MAX_CURSOR_SIZE 16			/* maximum cursor x and y size*/
/* max char height/width must be >= 16 and a multiple of sizeof(IMAGEBITS)*/
#define MAX_CHAR_HEIGHT	128			/* maximum text bitmap height*/
#define MAX_CHAR_WIDTH	128			/* maximum text bitmap width*/
#define	COORD_MIN	((COORD) -32768)	/* minimum coordinate value */
#define	COORD_MAX	((COORD) 32767)		/* maximum coordinate value */

/* compiled fonts, numbered for now*/
#define FONT_SYSTEM_VAR		0	/* winSystem*/
#define FONT_GUI_VAR		1	/* winMSSansSerif*/
#define FONT_OEM_FIXED		2	/* rom8x16*/
#define FONT_SYSTEM_FIXED	3	/* rom8x8*/
#define FONT_DEVICE_DEFAULT	2	/* rom8x16*/

/* Drawing modes*/
#define	MODE_SET	0	/* draw pixels as given (default) */
#define	MODE_XOR	1	/* draw pixels using XOR */
#define	MODE_OR		2	/* draw pixels using OR (notimp)*/
#define	MODE_AND	3	/* draw pixels using AND (notimp)*/
#define MODE_MAX	3

/* Mouse button bits in BUTTON*/
#define LBUTTON		04
#define MBUTTON		02
#define RBUTTON		01

/*
 * Pixel formats
 * Note the two pseudo pixel formats are never returned by display drivers,
 * but rather used as a data structure type in GrArea.  The other
 * types are both returned by display drivers and used as pixel packing
 * specifiers.
 */
#define PF_RGB		  0	/* pseudo, convert from packed 32 bit RGB*/
#define PF_PIXELVAL	  1	/* pseudo, no convert from packed PIXELVAL*/
#define PF_PALETTE	  2	/* pixel is packed 8 bits 1, 4 or 8 pal index*/
#define PF_TRUECOLOR0888  3	/* pixel is packed 32 bits 8/8/8 truecolor*/
#define PF_TRUECOLOR888	  4	/* pixel is packed 24 bits 8/8/8 truecolor*/
#define PF_TRUECOLOR565   5	/* pixel is packed 16 bits 5/6/5 truecolor*/
#define PF_TRUECOLOR332	  6	/* pixel is packed 8 bits 3/3/2 truecolor*/

/* typedefs etc. changable based on target system */

/* For the Nano-X server, it is important to use the correct PF_* value
 * for the SCREEN_PIXTYPE macro in order to match the hardware,
 * while the Nano-X clients that includes this file can get away with
 * a default pixel format of 24-bit color as the client will either:
 *    1) Use the PF_PIXELVAL native format when calling GrReadArea, in
 *       which case we have to have enough spare room to hold 32-bit
 *       pixlevalues (hence the default PF_TRUECOLOR0888 format), or
 *    2) Will use some other PF_* format, in which case the application
 *       is well aware of which pixel-format it uses and can avoid the
 *       device specific RGB2PIXEL and use RGB2PIXEL565 etc. instead,
 *       and specifiy the pixel fomar as PF_TRUECOLOR565 etc. when
 *       calling the GrArea function(s).
 */
#undef _SCREEN_PIXTYPE_OK
#ifndef SCREEN_PIXTYPE
#define SCREEN_PIXTYPE PF_TRUECOLOR0888
#endif

#if defined(__AS386_16__)
/* Force 8 bit palettized display for ELKS*/
#undef SCREEN_PIXTYPE
#define SCREEN_PIXTYPE PF_PALETTE
#endif

#if (SCREEN_PIXTYPE == PF_TRUECOLOR888) || (SCREEN_PIXTYPE == PF_TRUECOLOR0888)
typedef unsigned long PIXELVAL;
#define RGB2PIXEL(r,g,b)	RGB2PIXEL888(r,g,b)
#define PIXEL2RED(p)		PIXEL888RED(p)
#define PIXEL2GREEN(p)		PIXEL888GREEN(p)
#define PIXEL2BLUE(p)		PIXEL888BLUE(p)
#define _SCREEN_PIXTYPE_OK
#endif

#if SCREEN_PIXTYPE == PF_TRUECOLOR565
typedef unsigned short PIXELVAL;
#define RGB2PIXEL(r,g,b)	RGB2PIXEL565(r,g,b)
#define PIXEL2RED(p)		PIXEL565RED(p)
#define PIXEL2GREEN(p)		PIXEL565GREEN(p)
#define PIXEL2BLUE(p)		PIXEL565BLUE(p)
#define _SCREEN_PIXTYPE_OK
#endif

#if SCREEN_PIXTYPE == PF_TRUECOLOR332
typedef unsigned char PIXELVAL;
#define RGB2PIXEL(r,g,b)	RGB2PIXEL332(r,g,b)
#define PIXEL2RED(p)		PIXEL332RED(p)
#define PIXEL2GREEN(p)		PIXEL332GREEN(p)
#define PIXEL2BLUE(p)		PIXEL332BLUE(p)
#define _SCREEN_PIXTYPE_OK
#endif

#if SCREEN_PIXTYPE == PF_PALETTE
typedef unsigned char PIXELVAL;
#define _SCREEN_PIXTYPE_OK
#endif

#ifndef _SCREEN_PIXTYPE_OK
#error "Preprocessor macro SCREEN_PIXTYPE has invalid PF_* value"
#endif

typedef int		COORD;		/* device coordinates*/
typedef int		MODE;		/* drawing mode*/
typedef unsigned long	COLORVAL;	/* device-independent color value*/
typedef unsigned int	BUTTON;		/* mouse button mask*/
typedef unsigned int	MODIFIER;	/* keyboard modifier mask (CTRL/SHIFT)*/
typedef int		FONTID;		/* font number, 0=default font*/
typedef unsigned short	IMAGEBITS;	/* bitmap image unit size*/

#ifndef _WINDEF_H

#ifndef __ITRON_TYPES_h_ /* FIXME RTEMS hack*/
typedef int		BOOL;		/* boolean value*/
#endif

typedef unsigned char 	UCHAR;		/* text buffer*/
#endif

/* IMAGEBITS macros*/
#define	IMAGE_SIZE(width, height)  ((height) * (((width) + IMAGE_BITSPERIMAGE - 1) / IMAGE_BITSPERIMAGE))
#define IMAGE_WORDS(x)		(((x)+15)/16)
#define IMAGE_BYTES(x)		(((x)+7)/8)
#define	IMAGE_BITSPERIMAGE	(sizeof(IMAGEBITS) * 8)
#define	IMAGE_FIRSTBIT		((IMAGEBITS) 0x8000)
#define	IMAGE_NEXTBIT(m)	((IMAGEBITS) ((m) >> 1))
#define	IMAGE_TESTBIT(m)	((m) & IMAGE_FIRSTBIT)	  /* use with shiftbit*/
#define	IMAGE_SHIFTBIT(m)	((IMAGEBITS) ((m) << 1))  /* for testbit*/

/* GetScreenInfo structure*/
typedef struct {
	COORD 	 rows;		/* number of rows on screen */
	COORD 	 cols;		/* number of columns on screen */
	int 	 xdpcm;		/* dots/centimeter in x direction */
	int 	 ydpcm;		/* dots/centimeter in y direction */
	int	 planes;	/* hw # planes*/
	int	 bpp;		/* hw bpp*/
	long	 ncolors;	/* hw number of colors supported*/
	int 	 fonts;		/* number of built-in fonts */
	BUTTON 	 buttons;	/* buttons which are implemented */
	MODIFIER modifiers;	/* modifiers which are implemented */
	int	 pixtype;	/* format of pixel value*/
} SCREENINFO, *PSCREENINFO;

/* GetFontInfo structure*/
typedef struct {
	int 	font;		/* font number */
	int 	height;		/* height of font */
	int 	maxwidth;	/* maximum width of any char */
	int 	baseline;	/* baseline of font */
	BOOL	fixed;		/* TRUE if font is fixed width */
	UCHAR	widths[256];	/* table of character widths */
} FONTINFO, *PFONTINFO;

/* In-core proportional/fixed font structure*/
typedef struct {
	char *		name;		/* font name*/
	int		maxwidth;	/* max width in pixels*/
	int		height;		/* height in pixels*/
	int		firstchar;	/* first character in bitmap*/
	int		size;		/* font size in characters*/
	IMAGEBITS *	bits;		/* 16-bit right-padded bitmap data*/
	unsigned short *offset;		/* 256 offsets into bitmap data*/
	unsigned char *	width;		/* 256 character widths or NULL*/
					/* if non-proportional*/
} FONT, *PFONT;

/* In-core software cursor structure*/
typedef struct {
	int		width;			/* cursor width in pixels*/
	int		height;			/* cursor height in pixels*/
	COORD		hotx;			/* relative x pos of hot spot*/
	COORD		hoty;			/* relative y pos of hot spot*/
	COLORVAL	fgcolor;		/* foreground color*/
	COLORVAL	bgcolor;		/* background color*/
	IMAGEBITS	image[MAX_CURSOR_SIZE];	/* cursor image bits*/
	IMAGEBITS	mask[MAX_CURSOR_SIZE];	/* cursor mask bits*/
} SWCURSOR, *PSWCURSOR;

/* In-core color palette structure*/
typedef struct {
	UCHAR	r;
	UCHAR	g;
	UCHAR	b;
} RGBENTRY;

/* This structure is used to pass parameters into the low
 * level device driver functions.
 */
typedef struct {
	COORD dstx, dsty, dstw, dsth, dst_linelen;
	COORD srcx, srcy, src_linelen;
	void *pixels, *misc;
	PIXELVAL bg_color, fg_color;
	int gr_usebg;
} driver_gc_t;

/* Operations for the Blitter/Area functions */
#define PSDOP_COPY	0
#define PSDOP_COPYALL	1
#define PSDOP_COPYTRANS 2
#define PSDOP_ALPHAMAP	3
#define PSDOP_ALPHACOL	4
#define PSDOP_PIXMAP_COPYALL	5

/* ROP blitter opcodes*/
#define ROP_EXTENSION		0xff000000L	/* rop extension bits*/
#define ROP_SRCCOPY		0x00000000L	/* copy src to dst*/
#define ROP_CONSTANTALPHA	0x01000000L	/* alpha value in low 8 bits*/
/*
 * Interface to Screen Device Driver
 * This structure is also allocated for memory (offscreen) drawing.
 */
typedef struct _screendevice *PSD;
typedef struct _screendevice {
	COORD	xres;		/* X screen res (real) */
	COORD	yres;		/* Y screen res (real) */
	COORD	xvirtres;	/* X drawing res (will be flipped in portrait mode) */
	COORD	yvirtres;	/* Y drawing res (will be flipped in portrait mode) */
	int	planes;		/* # planes*/
	int	bpp;		/* # bpp*/
	int	linelen;	/* line length in bytes for bpp 1,2,4,8*/
				/* line length in pixels for bpp 16, 32*/
	int	size;		/* size of memory allocated*/
	long	ncolors;	/* # screen colors*/
	int	pixtype;	/* format of pixel value*/
	int	flags;		/* device flags*/
	void *	addr;		/* address of memory allocated (memdc or fb)*/

	int	(*Open)(PSD psd);
	void	(*Close)(PSD psd);
	void	(*GetScreenInfo)(PSD psd,PSCREENINFO psi);
	void	(*SetPalette)(PSD psd,int first,int count,RGBENTRY *pal);
	void	(*DrawPixel)(PSD psd,COORD x,COORD y,PIXELVAL c);
	PIXELVAL(*ReadPixel)(PSD psd,COORD x,COORD y);
	void	(*DrawHorzLine)(PSD psd,COORD x,COORD x2,COORD y,PIXELVAL c);
	void	(*DrawVertLine)(PSD psd,COORD x,COORD y1,COORD y2,PIXELVAL c);
	void	(*FillRect)(PSD psd,COORD x,COORD y,COORD w,COORD h,PIXELVAL c);
	/*void	(*DrawText)(PSD psd,COORD x,COORD y,const UCHAR *str,int count,
			PIXELVAL fg, FONTID fontid);*/
	BOOL	(*GetFontInfo)(PSD psd,FONTID fontid,PFONTINFO pfontinfo);
	void	(*GetTextSize)(PSD psd,const UCHAR *str,int cc,COORD *retwd,
			COORD *retht,FONTID fontid);
	void	(*GetTextBits)(PSD psd,UCHAR ch,IMAGEBITS *retmap,COORD *retwd,
			COORD *retht,FONTID fontid);
	void	(*Blit)(PSD destpsd,COORD destx,COORD desty,COORD w,COORD h,
			PSD srcpsd,COORD srcx,COORD srcy,int op);
	void	(*PreSelect)(PSD psd);
	void	(*DrawArea)(PSD psd, driver_gc_t *gc, int op);
} SCREENDEVICE;

/* PSD flags*/
#define	PSF_SCREEN	0x01	/* screen device*/
#define PSF_MEMORY	0x02	/* memory device*/
#define PSF_HAVEBLIT	0x04	/* have bitblit*/
#define PSF_PORTRAIT	0x08	/* in portrait mode*/
#define PSF_HAVEOP_COPY 0x10	/* psd->DrawArea can do Area copy */

/* Interface to Mouse Device Driver*/
typedef struct _mousedevice {
	int	(*Open)(struct _mousedevice *);
	void	(*Close)(void);
	BUTTON	(*GetButtonInfo)(void);
	void	(*GetDefaultAccel)(int *pscale,int *pthresh);
	int	(*Read)(COORD *dx,COORD *dy,COORD *dz,BUTTON *bp);
	int	(*Poll)(void);		/* not required if have select()*/
} MOUSEDEVICE;

/* Interface to Keyboard Device Driver*/
typedef struct _kbddevice {
	int  (*Open)(struct _kbddevice *pkd);
	void (*Close)(void);
	void (*GetModifierInfo)(MODIFIER *modifiers);
	int  (*Read)(UCHAR *buf,MODIFIER *modifiers);
	int  (*Poll)(void);		/* not required if have select()*/
} KBDDEVICE;

/* point structure for polygons*/
typedef struct {
	COORD 	x;		/* x coordinate*/
	COORD 	y;		/* y coordinate*/
} XYPOINT;

/* Clip rectangle: drawing allowed if point within rectangle*/
typedef struct {
	COORD 	x;		/* x coordinate of top left corner */
	COORD 	y;		/* y coordinate of top left corner */
	COORD 	width;		/* width of rectangle */
	COORD 	height;		/* height of rectangle */
} CLIPRECT;

/* Clip areas*/
#define CLIP_VISIBLE		0
#define CLIP_INVISIBLE		1
#define CLIP_PARTIAL		2

/*
 * Rectangle structure for region clipping.
 * Note this must match the Windows RECT structure.
 */
typedef struct {
	COORD	left;
	COORD	top;
	COORD	right;
	COORD	bottom;
} RECT;

/* dynamically allocated multi-rectangle clipping region*/
typedef struct {
	int	size;		/* malloc'd # of rectangles*/
	int	numRects;	/* # rectangles in use*/
	int	type; 		/* region type*/
	RECT *	rects;		/* rectangle array*/
	RECT	extents;	/* bounding box of region*/
} CLIPREGION;

/* region types */
#define ERRORREGION	0
#define NULLREGION	1
#define SIMPLEREGION	2
#define COMPLEXREGION	3

#ifndef TRUE
#define TRUE			1
#endif
#ifndef FALSE
#define FALSE			0
#endif

/* min, max*/
#define	min(a,b)		((a) < (b) ? (a) : (b))
#define	max(a,b) 		((a) > (b) ? (a) : (b))

/* color and palette defines*/
#define RGB(r,g,b)	((COLORVAL)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
#define F_PALINDEX	0x01000000
#define PALINDEX(x)	((COLORVAL)F_PALINDEX | (x))
#define RGBDEF(r,g,b)	{r, g, b}

#if 1 | BIGENDIAN
#define GETPALENTRY(pal,index) ((unsigned long)(pal[index].r |\
				(pal[index].g << 8) | (pal[index].b << 16)))
#else
#define GETPALENTRY(pal,index) ((*(unsigned long *)&pal[index]) & 0x00ffffff)
#endif

#define REDVALUE(rgb)	((rgb) & 0xff)
#define GREENVALUE(rgb) (((rgb) >> 8) & 0xff)
#define BLUEVALUE(rgb)	(((rgb) >> 16) & 0xff)

/* Truecolor color conversion and extraction macros*/
/*
 * Conversion from RGB to PIXELVAL
 */
/* create 24 bit 8/8/8 format pixel (0x00RRGGBB) from RGB triplet*/
#define RGB2PIXEL888(r,g,b)	\
	(((r) << 16) | ((g) << 8) | (b))

/* create 16 bit 5/6/5 format pixel from RGB triplet */
#define RGB2PIXEL565(r,g,b)	\
	((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

/* create 8 bit 3/3/2 format pixel from RGB triplet*/
#define RGB2PIXEL332(r,g,b)	\
	(((r) & 0xe0) | (((g) & 0xe0) >> 3) | (((b) & 0xc0) >> 6))

/*
 * Conversion from COLORVAL to PIXELVAL
 */
/* create 24 bit 8/8/8 format pixel from RGB colorval (0x00BBGGRR)*/
#define COLOR2PIXEL888(c)	\
	((((c) & 0xff) << 16) | ((c) & 0xff00) | (((c) & 0xff0000) >> 16))

/* create 16 bit 5/6/5 format pixel from RGB colorval (0x00BBGGRR)*/
#define COLOR2PIXEL565(c)	\
	((((c) & 0xf8) << 8) | (((c) & 0xfc00) >> 5) | (((c) & 0xf80000) >> 19))

/* create 8 bit 3/3/2 format pixel from RGB colorval (0x00BBGGRR)*/
#define COLOR2PIXEL332(c)	\
	(((c) & 0xe0) | (((c) & 0xe000) >> 11) | (((c) & 0xc00000) >> 22))

/*
 * Conversion from PIXELVAL to red, green or blue components
 */
/* return 8/8/8 bit r, g or b component of 24 bit pixelval*/
#define PIXEL888RED(pixelval)		(((pixelval) >> 16) & 0xff)
#define PIXEL888GREEN(pixelval)		(((pixelval) >> 8) & 0xff)
#define PIXEL888BLUE(pixelval)		((pixelval) & 0xff)

/* return 5/6/5 bit r, g or b component of 16 bit pixelval*/
#define PIXEL565RED(pixelval)		(((pixelval) >> 11) & 0x1f)
#define PIXEL565GREEN(pixelval)		(((pixelval) >> 5) & 0x3f)
#define PIXEL565BLUE(pixelval)		((pixelval) & 0x1f)

/* return 3/3/2 bit r, g or b component of 8 bit pixelval*/
#define PIXEL332RED(pixelval)		(((pixelval) >> 5) & 0x07)
#define PIXEL332GREEN(pixelval)		(((pixelval) >> 2) & 0x07)
#define PIXEL332BLUE(pixelval)		((pixelval) & 0x03)

/*
 * Common colors - note any color including these may not be
 * available on palettized systems, and the system will
 * then use the nearest color already in the system palette,
 * or allocate a new color entry.
 * These colors are the first 16 entries in the std palette,
 * and are written to the system palette if writable.
 */
#define BLACK		RGB( 0  , 0  , 0   )
#define BLUE		RGB( 0  , 0  , 128 )
#define GREEN		RGB( 0  , 128, 0   )
#define CYAN		RGB( 0  , 128, 128 )
#define RED		RGB( 128, 0  , 0   )
#define MAGENTA		RGB( 128, 0  , 128 )
#define BROWN		RGB( 128, 64 , 0   )
#define LTGRAY		RGB( 192, 192, 192 )
#define GRAY		RGB( 128, 128, 128 )
#define LTBLUE		RGB( 0  , 0  , 255 )
#define LGREEN		RGB( 0  , 255, 0   )
#define LYCYAN		RGB( 0  , 255, 255 )
#define LTRED		RGB( 255, 0  , 0   )
#define LTMAGENTA	RGB( 255, 0  , 255 )
#define YELLOW		RGB( 255, 255, 0   )
#define WHITE		RGB( 255, 255, 255 )

#if 0000
/* colors assumed in first 16 palette entries*/
/* note: don't use palette indices if the palette may
 * be reloaded.  Use the RGB values instead.
 */
#define BLACK		PALINDEX(0)		/*   0,   0,   0*/
#define BLUE		PALINDEX(1)
#define GREEN		PALINDEX(2)
#define CYAN		PALINDEX(3)
#define RED		PALINDEX(4)
#define MAGENTA		PALINDEX(5)
#define BROWN		PALINDEX(6)
#define LTGRAY		PALINDEX(7)		/* 192, 192, 192*/
#define GRAY		PALINDEX(8)		/* 128, 128, 128*/
#define LTBLUE		PALINDEX(9)
#define LTGREEN		PALINDEX(10)
#define LTCYAN		PALINDEX(11)
#define LTRED		PALINDEX(12)
#define LTMAGENTA	PALINDEX(13)
#define YELLOW		PALINDEX(14)
#define WHITE		PALINDEX(15)		/* 255, 255, 255*/
#endif

/* other common colors*/
#define DKGRAY		RGB( 32,  32,  32)

/* In-core mono and color image structure*/
typedef struct {
	int		width;		/* image width in pixels*/
	int		height;		/* image height in pixels*/
	int		planes;		/* # image planes*/
	int		bpp;		/* bits per pixel (1, 4 or 8)*/
	int		compression;	/* compression algorithm*/
	int		palsize;	/* palette size*/
	RGBENTRY *	palette;	/* palette*/
	UCHAR *		imagebits;	/* image bits (dword right aligned)*/
} IMAGEHDR, *PIMAGEHDR;

/* GdMakePaletteConversionTable bLoadType types*/
#define LOADPALETTE	1	/* load image palette into system palette*/
#define MERGEPALETTE	2	/* merge image palette into system palette*/

/* entry points*/

/* devdraw.c*/
int	GdOpenScreen(void);
void	GdCloseScreen(void);
MODE	GdSetMode(MODE m);
BOOL	GdSetUseBackground(BOOL flag);
PIXELVAL GdSetForeground(PIXELVAL fg);
PIXELVAL GdSetBackground(PIXELVAL bg);
FONTID	GdSetFont(FONTID fontid);
void	GdResetPalette(void);
void	GdSetPalette(int first, int count, RGBENTRY *palette);
int	GdGetPalette(int first, int count, RGBENTRY *palette);
PIXELVAL GdFindColor(COLORVAL c);
PIXELVAL GdFindNearestColor(RGBENTRY *pal, int size, COLORVAL cr);
void	GdGetScreenInfo(PSD psd,PSCREENINFO psi);
BOOL	GdGetFontInfo(PSD psd,FONTID fontid, PFONTINFO pfontinfo);
void	GdPoint(PSD psd,COORD x, COORD y);
void	GdLine(PSD psd,COORD x1,COORD y1,COORD x2,COORD y2,BOOL bDrawLastPoint);
void	GdRect(PSD psd,COORD x, COORD y, COORD width, COORD height);
void	GdFillRect(PSD psd,COORD x, COORD y, COORD width, COORD height);
void	GdGetTextSize(PSD psd,char *str, int cc, COORD *pwidth, COORD *pheight);
void	GdText(PSD psd,COORD x,COORD y,const UCHAR *str,int count,
		BOOL fBottomAlign);
void	GdBitmap(PSD psd,COORD x,COORD y,COORD width,COORD height,
		IMAGEBITS *imagebits);
BOOL	GdColorInPalette(COLORVAL cr,RGBENTRY *palette,int palsize);
void	GdMakePaletteConversionTable(RGBENTRY *palette,int palsize,
		PIXELVAL *convtable,int fLoadType);
void	GdDrawImage(PSD psd,COORD x, COORD y, PIMAGEHDR pimage);
void	GdEllipse(PSD psd,COORD x, COORD y, COORD rx, COORD ry);
void	GdFillEllipse(PSD psd,COORD x, COORD y, COORD rx, COORD ry);
void	GdPoly(PSD psd,int count, XYPOINT *points);
void	GdFillPoly(PSD psd,int count, XYPOINT *points);
void	GdReadArea(PSD psd,COORD x,COORD y,COORD width,COORD height,
		PIXELVAL *pixels);
void	GdArea(PSD psd,COORD x,COORD y,COORD width,COORD height,void *pixels,
		int pixtype);
void	GdCopyArea(PSD psd,COORD srcx,COORD srcy,COORD width,COORD height,
		COORD destx, COORD desty);
void	GdBlit(PSD dstpsd, COORD dstx, COORD dsty, COORD width, COORD height,
		PSD srcpsd, COORD srcx, COORD srcy, int rop);
extern SCREENDEVICE scrdev;

/* devclip.c*/
void 	GdSetClipRects(int count,CLIPRECT *table);
BOOL	GdClipPoint(COORD x,COORD y);
int	GdClipArea(COORD x1, COORD y1, COORD x2, COORD y2);
extern COORD clipminx, clipminy, clipmaxx, clipmaxy;

/* devrgn.c - multi-rectangle region clipping entry points*/
BOOL GdPtInRegion(CLIPREGION *rgn, COORD x, COORD y);
BOOL GdRectInRegion(CLIPREGION *rgn, const RECT *rect);
CLIPREGION *GdAllocClipRegion(void);
void GdDestroyClipRegion(CLIPREGION* pReg);
void GdUnionRectWithRegion(const RECT *rect, CLIPREGION *rgn);
void GdCopyRegion(CLIPREGION *d, CLIPREGION *s);
void GdIntersectRegion(CLIPREGION *d, CLIPREGION *s1, CLIPREGION *s2);
void GdUnionRegion(CLIPREGION *d, CLIPREGION *s1, CLIPREGION *s2);
void GdSubtractRegion(CLIPREGION *d, CLIPREGION *s1, CLIPREGION *s2);
void GdXorRegion(CLIPREGION *d, CLIPREGION *s1, CLIPREGION *s2);

/* devmouse.c*/
int	GdOpenMouse(void);
void	GdCloseMouse(void);
void	GdGetButtonInfo(BUTTON *buttons);
void	GdRestrictMouse(COORD newminx,COORD newminy,COORD newmaxx,
		COORD newmaxy);
void	GdSetAccelMouse(int newthresh, int newscale);
void	GdMoveMouse(COORD newx, COORD newy);
int	GdReadMouse(COORD *px, COORD *py, BUTTON *pb);
void	GdMoveCursor(COORD x, COORD y);
void	GdSetCursor(PSWCURSOR pcursor);
int 	GdShowCursor(void);
int 	GdHideCursor(void);
void	GdCheckCursor(COORD x1,COORD y1,COORD x2,COORD y2);
void 	GdFixCursor(void);
extern MOUSEDEVICE mousedev;

/* devkbd.c*/
int  	GdOpenKeyboard(void);
void 	GdCloseKeyboard(void);
void 	GdGetModifierInfo(MODIFIER *modifiers);
int  	GdReadKeyboard(UCHAR *buf, MODIFIER *modifiers);
extern KBDDEVICE kbddev;

/* devimage.c */
void GdJPEG (	PSD psd, COORD destx, COORD desty, COORD width, COORD height,
		BOOL fast_grayscale, char* filename);
void GdBMP (	PSD psd, COORD destx, COORD desty, COORD width, COORD height,
		char* filename);

/* no assert() in MSDOS or ELKS...*/
#if MSDOS | ELKS
#undef assert
#define assert(x)
#endif

/* RTEMS requires rtems_main()*/
#if __rtems__
#define main	rtems_main
#endif

#ifndef __rtems__
#define HAVESELECT	1	/* has select system call*/
#endif

#endif /*_DEVICE_H*/
