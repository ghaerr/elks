/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Framebuffer Video Driver for MicroWindows
 *
 * Portions used from Ben Pfaff's BOGL <pfaffben@debian.org>
 *
 * Note: modify select_driver() to add new framebuffer drivers
 */
#define _GNU_SOURCE 1
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "../device.h"
#include "fb.h"

#ifndef FB_TYPE_VGA_PLANES
#define FB_TYPE_VGA_PLANES 4
#endif

/* global vars*/
int	drawing;
FBENTRY	*fbprocs;

/* static variables*/
static int fb;			/* Framebuffer file handle. */
static int tty;			/* Tty file handle. */
static struct vt_mode mode;	/* Terminal mode. */
static int visible;		/* Is our VT visible? */
static int status;		/* 0=never inited, 1=once inited, 2=inited. */
static FBENTRY *savfbprocs;	/* copy of fb driver procs*/
static short saved_red[16];	/* original hw palette*/
static short saved_green[16];
static short saved_blue[16];

/* local functions*/
static FBENTRY *select_driver(PSD psd, int type, int visual);
static void 	ioctl_getpalette(int start, int len, short *red, short *green,
			short *blue);
static void	ioctl_setpalette(int start, int len, short *red, short *green,
			short *blue);
static void  	draw_enable(void);
static void 	draw_disable(void);
static void 	vt_switch(int);

/* init framebuffer*/
int
fb_init(PSD psd)
{
	char *	env;
	int	fbtype, fbvisual;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;
	//struct vt_stat vts;

	assert(status < 2);

	/* locate and open framebuffer, get info*/
	if(!(env = getenv("FRAMEBUFFER")))
		env = "/dev/fb0";
	fb = open(env, O_RDWR);
	if(fb < 0)
		return failmsg("opening %s: %m", env);
	if(ioctl(fb, FBIOGET_FSCREENINFO, &fb_fix) == -1 ||
		ioctl(fb, FBIOGET_VSCREENINFO, &fb_var) == -1)
			return failmsg("reading screen info: %m");

	/* set global framebuffer info*/
	psd->xres = fb_var.xres;
	psd->yres = fb_var.yres;
	psd->planes = 1;	/* assume 1 plane unless overridden*/
	psd->bpp = fb_var.bits_per_pixel;
	/* set linelen to byte length, possibly converted later*/
	psd->linelen = fb_fix.line_length;
	psd->ncolors = 1 << fb_var.bits_per_pixel;
	fbtype = fb_fix.type;
	fbvisual = fb_fix.visual;
	/*printf("%dx%dx%d linelen %d type %d visual %d\n", psd->xres,
	 	psd->yres, psd->ncolors, psd->linelen, fbtype, fbvisual);*/

	/* determine framebuffer driver and enable draw procedures*/
	if(!(savfbprocs = select_driver(psd, fbtype, fbvisual)))
		return failmsg("unknown screen type %d visual %d ncolors %d",
			fbtype, fbvisual, psd->ncolors);
	draw_enable();

	/* open tty, get info*/
	tty = open ("/dev/tty0", O_RDWR);
	if(tty < 0) {
		close(fb);
		return failmsg("opening /dev/tty0: %m");
	}
	//if(ioctl (tty, VT_GETSTATE, &vts) == -1)
		//return failmsg("can't get VT state: %m");
	//tty_no = vts.v_active;

	/* setup new tty mode*/
	if(ioctl (tty, VT_GETMODE, &mode) == -1)
		return failmsg("can't get VT mode: %m");
	mode.mode = VT_PROCESS;
	mode.relsig = SIGUSR2;
	mode.acqsig = SIGUSR2;
	signal (SIGUSR2, vt_switch);
	if(ioctl (tty, VT_SETMODE, &mode) == -1)
		return failmsg("can't set VT mode: %m");
	if(ioctl (tty, KDSETMODE, KD_GRAPHICS) == -1)
		return failmsg("setting graphics mode: %m");

	/* call driver init procedure to calc map size, map framebuffer*/
	if(!fbprocs->init(psd))
		return 0;
	psd->size = (psd->size + getpagesize () - 1)
			/ getpagesize () * getpagesize ();
	psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE,MAP_SHARED,fb,0);
	if(psd->addr == NULL || psd->addr == (unsigned char *)-1)
		return failmsg("mmaping %s: %m", env);
	psd->flags = PSF_SCREEN | PSF_HAVEBLIT;

	/* save original palette*/
	ioctl_getpalette(0, 16, saved_red, saved_green, saved_blue);

	status = 2;
	return 1;
}

/* close framebuffer*/
void
fb_exit(PSD psd)
{
	/* if not opened, return*/
	if(status != 2)
		return;
	status = 1;

  	/* reset hw palette*/
	ioctl_setpalette(0, 16, saved_red, saved_green, saved_blue);

	/* unmap framebuffer*/
	munmap(psd->addr, psd->size);

	/* reset tty modes*/
	signal(SIGUSR2, SIG_DFL);
	ioctl(tty, KDSETMODE, KD_TEXT);
	mode.mode = VT_AUTO;
	mode.relsig = 0;
	mode.acqsig = 0;
	ioctl(tty, VT_SETMODE, &mode);

	/* close tty and framebuffer*/
	close(tty);
	close(fb);
}

/* Select driver from fb type and visual*/
/* modify this procedure to add a new framebuffer driver*/
static FBENTRY *
select_driver(PSD psd, int type, int visual)
{
	FBENTRY * driver = NULL;
	extern FBENTRY	fblinear1;
	extern FBENTRY	fblinear2;
	extern FBENTRY	fblinear4;
	extern FBENTRY	fblinear8;
	extern FBENTRY	fblinear16;
	extern FBENTRY	fblinear32;
#ifdef FBVGA
	extern FBENTRY	fbvgaplan4;

	/* currently, the driver is selected from the fbtype alone*/
	if(type == FB_TYPE_VGA_PLANES) {
		psd->planes = 4;
		driver = &fbvgaplan4;
	}
#endif

	if(type == FB_TYPE_PACKED_PIXELS) {
		if(psd->ncolors == 2)
			driver = &fblinear1;
		else if(psd->ncolors == 4)
			driver = &fblinear2;
		else if(psd->ncolors == 16)
			driver = &fblinear4;
		else if(psd->ncolors == 256)
			driver = &fblinear8;
		else if(psd->ncolors == 65536)
			driver = &fblinear16;
		else if(psd->ncolors > 65536)
			driver = &fblinear32;
	}
	if(!driver)
		return NULL;

	/* set pixel format*/
	if(visual == FB_VISUAL_TRUECOLOR) {
		switch(psd->ncolors) {
		case 1 << 8:
			psd->pixtype = PF_TRUECOLOR332;
			break;

		case 1 << 16:
			psd->pixtype = PF_TRUECOLOR565;
			break;

		case 1 << 24:
			psd->pixtype = PF_TRUECOLOR24;
			break;

		default:
			failmsg("Unsupported %d color truecolor framebuffer\n",
				psd->ncolors);
			return NULL;
		}
	} else
		psd->pixtype = PF_PALETTE;

	/* return driver selected*/
	return driver;
}

/* convert MicroGDI palette to framebuffer format and set it*/
void
fb_setpalette(PSD psd,int first, int count, RGBENTRY *palette)
{
	int 	i;
	short 	red[count];
	short 	green[count];
	short 	blue[count];

	/* convert palette to framebuffer format*/
	for(i=0; i < count; i++) {
		RGBENTRY *p = &palette[i];

		/* grayscale computation:
		 * red[i] = green[i] = blue[i] =
		 *	(p->r * 77 + p->g * 151 + p->b * 28);
		 */
		red[i] = p->r << 8;
		green[i] = p->g << 8;
		blue[i] = p->b << 8;
	}
	ioctl_setpalette(first, count, red, green, blue);
}

/* get framebuffer palette*/
static void
ioctl_getpalette(int start, int len, short *red, short *green,
	short *blue)
{
	struct fb_cmap cmap;

	cmap.start = start;
	cmap.len = len;
	cmap.red = red;
	cmap.green = green;
	cmap.blue = blue;
	cmap.transp = NULL;

	ioctl(fb, FBIOGETCMAP, &cmap);
}

/* set framebuffer palette*/
static void
ioctl_setpalette(int start, int len, short *red, short *green,
	short *blue)
{
	struct fb_cmap cmap;

	cmap.start = start;
	cmap.len = len;
	cmap.red = red;
	cmap.green = green;
	cmap.blue = blue;
	cmap.transp = NULL;

	ioctl(fb, FBIOPUTCMAP, &cmap);
}

static void
draw_enable(void)
{
	visible = 1;
	fbprocs = savfbprocs;
}

static void
draw_disable(void)
{
	visible = 0;
	fbprocs = &fbnull;
}

/* Signal handler called when kernel switches to or from our tty*/
static void
vt_switch(int sig)
{
    struct itimerval duration;

    signal(SIGUSR2, vt_switch);

    /* If a drawing function is in progress then we cannot mode
     * switch right now because the drawing function would continue to
     * scribble on the screen after the switch.  So disable further
     * drawing and schedule an alarm to try again in .1 second.
     */
    if(drawing) {
    	draw_disable ();

    	signal(SIGALRM, vt_switch);

	duration.it_interval.tv_sec = 0;
	duration.it_interval.tv_usec = 0;
	duration.it_value.tv_sec = 0;
	duration.it_value.tv_usec = 100000;
    	setitimer (ITIMER_REAL, &duration, NULL);
    	return;
    }

    if(visible) {
    	draw_disable ();

	if(ioctl (tty, VT_RELDISP, 1) == -1)
	    failmsg("can't switch away from VT: %m");
    } else {
    	draw_enable ();

	if(ioctl (tty, VT_RELDISP, VT_ACKACQ) == -1)
		failmsg("can't acknowledge VT switch: %m");
    }
}

/* output error message and return 0*/
int
failmsg(const char *format, ...)
{
    va_list 	args;
    char 	error[400];

    va_start(args, format);
    vsprintf(error, format, args);
    va_end(args);
    printf("Error %s\n", error);
    return 0;
}
