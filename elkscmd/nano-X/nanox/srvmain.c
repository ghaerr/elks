/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Main module of graphics server.
 */
#include <autoconf.h>           /* for CONFIG_ options */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include "serv.h"
#if UNIX
#include <unistd.h>
#include <sys/time.h>
#endif
#if ELKS
#include <linuxmt/posix_types.h>
#include <linuxmt/time.h>
#endif

/*
 * External definitions defined here.
 */
GR_WINDOW_ID	cachewindowid;		/* cached window id */
GR_GC_ID	cachegcid;		/* cached graphics context id */
GR_WINDOW	*cachewp;		/* cached window pointer */
GR_GC		*cachegcp;		/* cached graphics context */
GR_WINDOW	*listwp;		/* list of all windows */
GR_WINDOW	*rootwp;		/* root window pointer */
GR_GC		*listgcp;		/* list of all gc */
GR_GC		*curgcp;		/* currently enabled gc */
GR_WINDOW	*clipwp;		/* window clipping is set for */
GR_WINDOW	*focuswp;		/* focus window for keyboard */
GR_WINDOW	*mousewp;		/* window mouse is currently in */
GR_WINDOW	*grabbuttonwp;		/* window grabbed by button */
GR_CURSOR	*curcursor;		/* currently enabled cursor */
GR_COORD	cursorx;		/* current x position of cursor */
GR_COORD	cursory;		/* current y position of cursor */
BUTTON	curbuttons;			/* current state of buttons */
GR_CLIENT	*curclient;		/* client currently executing for */
GR_EVENT_LIST	*eventfree;		/* list of free events */
GR_BOOL		focusfixed;		/* TRUE if focus is fixed on a window */
GR_SCREEN_INFO	sinfo;			/* screen information */
GR_FONT_INFO	curfont;		/* current font information */
int		current_fd;		/* the fd of the client we are talking to */
int		connectcount;		/* number of connections to server */
GR_CLIENT	*root_client;		/* root entry of the client table */
GR_CLIENT	*current_client;	/* the client we are currently talking to */
int		keyb_fd;		/* the keyboard file descriptor */
int		mouse_fd;		/* the mouse file descriptor */
char		*curfunc;		/* the name of the current server function */

#if !NONETWORK
int		un_sock;		/* the server socket descriptor */

/*
 * This is the main server loop which initialises the server, services
 * the clients, and shuts the server down when there are no more clients.
 */
int
main(int argc, char *argv[])
{
	/* Attempt to initialise the server*/
	if(GsInitialize())
		exit(1);

	while(1)
		GsSelect(GR_TIMEOUT_BLOCK);
	return 0;
}
#endif

void
GsAcceptClientFd(int i)
{
	GR_CLIENT *client, *cl;

	if(!(client = malloc(sizeof(GR_CLIENT)))) {
		close(i);
		return;
	}

	client->id = i;
	client->eventhead = NULL;
	client->eventtail = NULL;
	client->errorevent.type = GR_EVENT_TYPE_NONE;
	client->next = NULL;
	client->waiting_for_event = FALSE;

	if(connectcount++ == 0)
		root_client = client;
	else {
		cl = root_client;
			while(cl->next)
				cl = cl->next;
		client->prev = cl;
		cl->next = client;
	}
}

/*
 * Open a connection from a new client to the server.
 * Returns -1 on failure.
 */
int
GsOpen(void)
{
#if NONETWORK
	/* Client calls this routine once.  We 
	 * init everything here
	 */
	if(GsInitialize() < 0)
		return -1;
	GsAcceptClientFd(999);
	curclient = root_client;
#endif
        return 1;
}

/*
 * Close the connection to the server.
 */
void
GsClose(void)
{
#if !NONETWORK
	GsDropClient(current_fd);
#endif
        if(--connectcount == 0)
		GsTerminate();
}

#if UNIX
#if NONETWORK
/*
 * Register the specified file descriptor to return an event
 * when input is ready.
 * FIXME: only one external file descriptor works
 */
static int regfd = -1;

void
GsRegisterInput(int fd)
{
	regfd = fd;
}
#endif /* NONETWORK*/
#endif /* UNIX*/

#if MSDOS
void
GsSelect(GR_TIMEOUT timeout)
{
	/* If mouse data present, service it*/
	if(mousedev.Poll())
		GsCheckMouseEvent();

	/* If keyboard data present, service it*/
	if(kbddev.Poll())
		GsCheckKeyboardEvent();

}
#endif

#if UNIX
void
GsSelect(GR_TIMEOUT timeout)
{
	fd_set	rfds;
	int 	e;
	int	setsize = 0;
	struct timeval to;

#if CONFIG_ARCH_PC98
	if (mousedev.Poll()) {
		GsCheckMouseEvent();
		return;
	}
#endif

	/* Set up the FDs for use in the main select(): */
	FD_ZERO(&rfds);
	if (mouse_fd >= 0) FD_SET(mouse_fd, &rfds);
	FD_SET(keyb_fd, &rfds);
	if (mouse_fd > setsize) setsize = mouse_fd;
	if (keyb_fd > setsize) setsize = keyb_fd;
#if NONETWORK
	/* handle registered input file descriptors*/
	if (regfd != -1) {
		FD_SET(regfd, &rfds);
		if (regfd > setsize) setsize = regfd;
	}
#else
	/* handle client socket connections*/
	FD_SET(un_sock, &rfds);
	if (un_sock > setsize) setsize = un_sock;
	curclient = root_client;
	while(curclient) {
		if(curclient->waiting_for_event && curclient->eventhead) {
			curclient->waiting_for_event = FALSE;
			GsGetNextEventWrapperFinish();
			return;
		}
		FD_SET(curclient->id, &rfds);
		if(curclient->id > setsize) setsize = curclient->id;
		curclient = curclient->next;
	}
#endif
	++setsize;

    /* convert wait timeout to timeval struct*/
#if CONFIG_ARCH_PC98
    if (timeout == GR_TIMEOUT_BLOCK)
        timeout = 10;
#endif
    if (timeout == GR_TIMEOUT_POLL) {
        to.tv_sec = 0;
        to.tv_usec = 0;
    } else {
        to.tv_sec = timeout / 1000;
        to.tv_usec = (timeout % 1000) * 1000;
    }

	/* Wait for some input on any of the fds in the set or a timeout: */
	if((e = select(setsize, &rfds, NULL, NULL, timeout == 0? NULL: &to)) >= 0) {
		
		/* If data is present on the mouse fd, service it: */
		if(mouse_fd >= 0 && FD_ISSET(mouse_fd, &rfds))
			GsCheckMouseEvent();

		/* If data is present on the keyboard fd, service it: */
		if(FD_ISSET(keyb_fd, &rfds))
			GsCheckKeyboardEvent();

#if NONETWORK
		/* If registered input descriptor, handle it*/
		if(regfd != -1 && FD_ISSET(regfd, &rfds)) {
			GR_EVENT_FDINPUT *	gp;
			gp = (GR_EVENT_FDINPUT *)GsAllocEvent(curclient);
			if(gp) {
				gp->type = GR_EVENT_TYPE_FDINPUT;
				gp->fd = regfd;
			}
		}
#else
		/* If a client is trying to connect, accept it: */
		if(FD_ISSET(un_sock, &rfds))
			GsAcceptClient();

		/* If a client is sending us a command, handle it: */
		curclient = root_client;
		while(curclient) {
			if(FD_ISSET(curclient->id, &rfds))
				GsHandleClient(curclient->id);
			curclient = curclient->next;
		}
#endif

	} 
#if 0
	else if(!e) {
#if FRAMEBUFFER | BOGL
		if(fb_CheckVtChange())
			GsRedrawScreen();
#endif
	}
#endif
    else
		if(errno != EINTR)
			perror("Select() call in main failed");
}
#endif

/*
 * Initialize the graphics and mouse devices at startup.
 * Returns nonzero with a message printed if the initialization failed.
 */
int
GsInitialize(void)
{
	GR_WINDOW	*wp;		/* root window */
	static IMAGEBITS cursorbits[16] = {
	      0xe000, 0x9800, 0x8600, 0x4180,
	      0x4060, 0x2018, 0x2004, 0x107c,
	      0x1020, 0x0910, 0x0988, 0x0544,
	      0x0522, 0x0211, 0x000a, 0x0004
	};
	static IMAGEBITS cursormask[16] = {
	      0xe000, 0xf800, 0xfe00, 0x7f80,
	      0x7fe0, 0x3ff8, 0x3ffc, 0x1ffc,
	      0x1fe0, 0x0ff0, 0x0ff8, 0x077c,
	      0x073e, 0x021f, 0x000e, 0x0004
	};

	wp = (GR_WINDOW *) malloc(sizeof(GR_WINDOW));
	if (wp == NULL) {
		fprintf(stderr, "Cannot allocate root window\n");
		return -1;
	}

#if !NONETWORK
	if (GsOpenSocket() < 0) {
		perror("Cannot bind to named socket");
		free(wp);
		return -1;
	}
#endif

	if ((keyb_fd = GdOpenKeyboard()) < 0) {
		perror("Cannot initialise keyboard");
		/*GsCloseSocket();*/
		free(wp);
		return -1;
	}

	if ((mouse_fd = GdOpenMouse()) == -1) { /* -2 == mou_nul.c */
		/*perror("Cannot initialise mouse");*/
		/*GsCloseSocket();*/
		GdCloseKeyboard();
		free(wp);
		return -1;
	}

	if (GdOpenScreen() < 0) {
		perror("Cannot initialise screen");
		/*GsCloseSocket();*/
		GdCloseMouse();
		GdCloseKeyboard();
		free(wp);
		return -1;
	}

	/*
	 * Get screen dimensions for our own and the client's use,
	 * and the information about the default font.
	 */
	GdGetScreenInfo(&scrdev, &sinfo);
	GdGetFontInfo(&scrdev, FONT_OEM_FIXED, &curfont);
	GdGetModifierInfo(&sinfo.modifiers);
	GdGetButtonInfo(&sinfo.buttons);

	/*
	 * Initialize the root window.
	 */
	wp->id = GR_ROOT_WINDOW_ID;
	wp->parent = wp;
	wp->children = NULL;
	wp->siblings = NULL;
	wp->next = NULL;
	wp->x = 0;
	wp->y = 0;
	wp->width = sinfo.cols;
	wp->height = sinfo.rows;
	wp->bordersize = 0;
	wp->background = BLACK;
	wp->bordercolor = BLACK;
	wp->nopropmask = 0;
	wp->eventclients = NULL;
	wp->cursor = NULL;
	wp->mapped = GR_TRUE;
	wp->unmapcount = 0;
	wp->output = GR_TRUE;

	listwp = wp;
	rootwp = wp;
	focuswp = wp;
	mousewp = wp;
	focusfixed = GR_FALSE;

	/*
	 * Initialize and position the default cursor.
	 */
	curcursor = NULL;
	cursorx = -1;
	cursory = -1;
	GdShowCursor();
	GsMoveCursor(sinfo.cols / 2, sinfo.rows / 2);
	GsSetCursor(GR_ROOT_WINDOW_ID, 16, 16, 0, 0, WHITE, BLACK,
		cursorbits, cursormask);

#if FRAMEBUFFER | BOGL
	fb_InitVt();
#endif
	scrdev.FillRect(&scrdev, 0, 0, sinfo.cols-1, sinfo.rows-1,
		GdFindColor(BLACK));

	/*
	 * Finally tell the mouse driver some things.
	 */
	curbuttons = 0;
	/*GdSetAccelMouse(5, 3);*/
	GdRestrictMouse(0, 0, sinfo.cols - 1, sinfo.rows - 1);
	GdMoveMouse(sinfo.cols / 2, sinfo.rows / 2);
	GsFlush();

	/*
	 * All done.
	 */
	connectcount = 0;
	return 0;
}

/*
 * Here to close down the server.
 */
void
GsTerminate(void)
{
#if !NONETWORK
	GsCloseSocket();
#endif
	GdCloseScreen();
	GdCloseMouse();
	GdCloseKeyboard();
#if FRAMEBUFFER | BOGL
	fb_RedrawVt(vterm);
#endif
	exit(0);
}
