/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Framebuffer drivers header file for MicroWindows Video Drivers
 */

/* Linux framebuffer critical sections*/
#if LINUX
#define DRAWON		++drawing
#define DRAWOFF		--drawing
#else
#define DRAWON
#define DRAWOFF
#endif

typedef unsigned char *		ADDR8;
typedef unsigned short *	ADDR16;
typedef unsigned long *		ADDR32;

typedef struct {
	int	 (*init)(PSD psd);
	void 	 (*drawpixel)(PSD psd, COORD x, COORD y, PIXELVAL c);
	PIXELVAL (*readpixel)(PSD psd, COORD x, COORD y);
	void 	 (*drawhorzline)(PSD psd, COORD x1, COORD x2, COORD y,
			PIXELVAL c);
	void	 (*drawvertline)(PSD psd, COORD x, COORD y1, COORD y2,
			PIXELVAL c);
	void	 (*blit)(PSD destpsd, COORD destx, COORD desty, COORD w,
			COORD h, PSD srcpsd, COORD srcx, COORD srcy, int op);
} FBENTRY;

/* entry points*/
int  fb_init(PSD psd);
void fb_exit(PSD psd);
void fb_setpalette(PSD psd,int first, int count, RGBENTRY *palette);
int  failmsg(const char *format, ...);
#define fb_drawpixel(p,x,y,c)		(fbprocs->drawpixel)(p,x,y,c)
#define fb_readpixel(p,x,y)		(fbprocs->readpixel)(p,x,y)
#define fb_drawhorzline(p,x1,x2,y,c)	(fbprocs->drawhorzline)(p,x1,x2,y,c)
#define fb_drawvertline(p,x,y1,y2,c)	(fbprocs->drawvertline)(p,x,y1,y2,c)
#define fb_blit(dp,dx,dy,w,h,sp,sx,sy,op)(fbprocs->blit)(dp,dx,dy,w,h,sp,sx,sy,op)

/* global vars*/
extern int	drawing;
extern FBENTRY	*fbprocs;
extern FBENTRY	fbnull;

extern int 	gr_mode;	/* temp kluge*/
