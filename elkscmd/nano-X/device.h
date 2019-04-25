#ifndef _DEVICE_H
#define _DEVICE_H
/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Mid-level Screen, Mouse and Keyboard device driver API's
 */

/* Changeable limits */
#define	MAX_CLIPRECTS 	200			/* maximum clip rectangles */
#define	MAX_CURSOR_SIZE 16			/* maximum cursor x and y size*/
#define MAX_CHAR_HEIGHT	16			/* maximum text bitmap height*/
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

/* typedefs changable based on target system*/
typedef int		COORD;		/* device coordinates*/
typedef int		MODE;		/* drawing mode*/
typedef unsigned long	COLORVAL;	/* device-independent color value*/
#if ELKS
/* support up to 32 bit truecolor except on ELKS for size*/
typedef unsigned char	PIXELVAL;	/* pixel color value*/
#else
typedef unsigned long	PIXELVAL;	/* pixel color value*/
#endif
typedef unsigned int	BUTTON;		/* mouse button mask*/
typedef unsigned int	MODIFIER;	/* keyboard modifier mask (CTRL/SHIFT)*/
typedef int		FONTID;		/* font number, 0=default font*/
typedef unsigned short	IMAGEBITS;	/* bitmap image unit size*/

#ifndef _WINDEF_H
typedef int		BOOL;		/* boolean value*/
typedef unsigned char 	UCHAR;		/* text buffer*/
#endif

/* IMAGEBITS macros*/
#define	IMAGE_SIZE(width, height)  ((height) * (((width) + sizeof(IMAGEBITS) * 8 - 1) / (sizeof(IMAGEBITS) * 8)))
#define IMAGE_WORDS(x)		(((x)+15)/16)
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

/*
 * Interface to Screen Device Driver
 * This structure is also allocated for memory (offscreen) drawing.
 */
typedef struct _screendevice *PSD;
typedef struct _screendevice {
	COORD	xres;		/* X screen res*/
	COORD	yres;		/* Y screen res*/
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
} SCREENDEVICE;

/* PSD flags*/
#define	PSF_SCREEN	0x01	/* screen device*/
#define PSF_MEMORY	0x02	/* memory device*/
#define PSF_HAVEBLIT	0x04	/* have bitblit*/

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

/* note: the following macro only works for little-endian machines*/
#define GETPALENTRY(pal,index) ((*(unsigned long *)&pal[index]) & 0x00ffffff)

#define REDVALUE(rgb)	((rgb) & 0xff)
#define GREENVALUE(rgb) (((rgb) >> 8) & 0xff)
#define BLUEVALUE(rgb)	(((rgb) >> 16) & 0xff)

/* pixel formats*/
#define PF_PALETTE	0	/* pixel is 1, 4 or 8 bit palette index*/
#define PF_TRUECOLOR24	1	/* pixel is 24 bit 8/8/8 truecolor*/
#define PF_TRUECOLOR565 2       /* pixel is 16 bit 5/6/5 truecolor*/
#define PF_TRUECOLOR332	3	/* pixel is 8 bit 3/3/2 truecolor*/

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
void	GdArea(PSD psd,COORD x,COORD y,COORD width,COORD height,
		PIXELVAL *pixels);
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

/* no assert() in MSDOS or ELKS...*/
#if MSDOS | ELKS
#undef assert
#define assert(x)
#endif
#endif /*_DEVICE_H*/
