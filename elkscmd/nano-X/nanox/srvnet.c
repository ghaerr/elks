/*
 * Copyright (c) 1999 Alex Holden <alex@linuxhacker.org>
 * Portions Copyright (c) 1991 David I. Bell
 *
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * This is the server side of the network interface, which accepts
 * connections from clients, recieves functions from them, and dispatches
 * events to them.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#ifndef __linux__ 
#include <linuxmt/socket.h>
#include <linuxmt/un.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#endif
#include <sys/stat.h>
#include "serv.h"

extern	int		un_sock;
extern	GR_CLIENT	*root_client;
extern	int		current_fd;

/*
 * These are all wrapper functions which are used to read the arguments for and call the
 * relevant function.
 */

void GsOpenWrapper(void)
{
	GsOpen();

	GsPutCh(current_fd, GrRetOK);
}

void GsCloseWrapper(void)
{
	GsPutCh(current_fd, GrRetOK);

	GsClose();
}

void GsGetScreenInfoWrapper(void)
{
	GR_SCREEN_INFO si;

	GsGetScreenInfo(&si);

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, &si, sizeof(si));
}

void GsNewWindowWrapper(void)
{
	GR_WINDOW_ID wid, parent;
	GR_COORD x, y;
	GR_SIZE width, height, bordersize;
	GR_COLOR background, bordercolor;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &parent, sizeof(parent)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &x, sizeof(x)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &y, sizeof(y)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &width, sizeof(width)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &height, sizeof(height)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &bordersize, sizeof(bordersize)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &background, sizeof(background)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &bordercolor, sizeof(bordercolor)))
		return;

	wid = GsNewWindow(parent, x, y, width, height, bordersize, background, bordercolor);

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, &wid, sizeof(wid));
}

void GsNewInputWindowWrapper(void)
{
	GR_WINDOW_ID wid, parent;
	GR_COORD x, y;
	GR_SIZE width, height;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &parent, sizeof(parent)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &x, sizeof(x)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &y, sizeof(y)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &width, sizeof(width)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &height, sizeof(height)))
		return;

	wid = GsNewInputWindow(parent, x, y, width, height);

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, &wid, sizeof(wid));
}

void GsDestroyWindowWrapper(void)
{
	GR_WINDOW_ID wid;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &wid, sizeof(wid)))
		return;

	GsDestroyWindow(wid);

	GsPutCh(current_fd, GrRetOK);
}

void GsNewGCWrapper(void)
{
	GR_GC_ID gc = GsNewGC();

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, (void *) &gc, sizeof(gc));
}

void GsCopyGCWrapper(void)
{
	GR_GC_ID gci, gco;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &gci, sizeof(gci)))
		return;

	gco = GsCopyGC(gci);

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, (void *) &gco, sizeof(gco));
}

void GsGetGCInfoWrapper(void)
{
	GR_GC_INFO gc;
	GR_GC_ID g;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &g, sizeof(g)))
		return;

	GsGetGCInfo(g, &gc);

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, (void *) &gc, sizeof(gc));
}

void GsDestroyGCWrapper(void)
{
	GR_GC_ID gc;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &gc, sizeof(gc)))
		return;

	GsDestroyGC(gc);

	GsPutCh(current_fd, GrRetOK);
}

void GsMapWindowWrapper(void)
{
	GR_WINDOW_ID wid;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &wid, sizeof(wid)))
		return;

	GsMapWindow(wid);

	GsPutCh(current_fd, GrRetOK);
}

void GsUnmapWindowWrapper(void)
{
	GR_WINDOW_ID wid;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &wid, sizeof(wid)))
		return;

	GsUnmapWindow(wid);

	GsPutCh(current_fd, GrRetOK);
}

void GsRaiseWindowWrapper(void)
{
	GR_WINDOW_ID wid;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &wid, sizeof(wid)))
		return;

	GsRaiseWindow(wid);

	GsPutCh(current_fd, GrRetOK);
}

void GsLowerWindowWrapper(void)
{
	GR_WINDOW_ID wid;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &wid, sizeof(wid)))
		return;

	GsLowerWindow(wid);

	GsPutCh(current_fd, GrRetOK);
}

void GsMoveWindowWrapper(void)
{
	GR_WINDOW_ID wid;
	GR_COORD x;
	GR_COORD y;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &wid, sizeof(wid)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &x, sizeof(x)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &y, sizeof(y)))
		return;

	GsMoveWindow(wid, x, y);

	GsPutCh(current_fd, GrRetOK);
}

void GsResizeWindowWrapper(void)
{
	GR_WINDOW_ID wid;
	GR_SIZE width;
	GR_SIZE height;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &wid, sizeof(wid)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &width, sizeof(width)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &height, sizeof(height)))
		return;

	GsResizeWindow(wid, width, height);

	GsPutCh(current_fd, GrRetOK);
}

void GsGetWindowInfoWrapper(void)
{
	GR_WINDOW_ID wid;
	GR_WINDOW_INFO wi;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &wid, sizeof(wid)))
		return;

	GsGetWindowInfo(wid, &wi);

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, (void *) &wi, sizeof(wi));
}

void GsGetFontInfoWrapper(void)
{
	GR_FONT_INFO fi;
	GR_FONT font;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &font, sizeof(GR_FONT)))
		return;

	GsGetFontInfo(font, &fi);

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, &fi, sizeof(fi));
}

void GsSetFocusWrapper(void)
{
	GR_WINDOW_ID wid;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &wid, sizeof(wid)))
		return;

	GsSetFocus(wid);

	GsPutCh(current_fd, GrRetOK);
}

void GsSetBorderColorWrapper(void)
{
	GR_WINDOW_ID wid;
	GR_COLOR colour;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &wid, sizeof(wid)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &colour, sizeof(colour)))
		return;

	GsSetBorderColor(wid, colour);

	GsPutCh(current_fd, GrRetOK);
}

void GsClearWindowWrapper(void)
{
	GR_WINDOW_ID wid;
	GR_BOOL exposeflag;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &wid, sizeof(wid)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &exposeflag, sizeof(exposeflag)))
		return;

	GsClearWindow(wid, exposeflag);

	GsPutCh(current_fd, GrRetOK);
}

void GsSelectEventsWrapper(void)
{
	GR_WINDOW_ID wid;
	GR_EVENT_MASK eventmask;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &wid, sizeof(wid)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &eventmask, sizeof(eventmask)))
		return;

	GsSelectEvents(wid, eventmask);

	GsPutCh(current_fd, GrRetOK);	
}

void GsGetNextEventWrapper(void)
{
	GR_EVENT evt;

	/* first check if any event ready*/
	GsCheckNextEvent(&evt);
	if(evt.type == GR_EVENT_TYPE_NONE) {
		/* tell main loop to call Finish routine on event*/
		curclient->waiting_for_event = TRUE;
		return;
	}

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, (void *) &evt, sizeof(evt));
}

/* Complete the GrGetNextEvent call from client.
 * The client is still waiting on a read at this point.
 */
void GsGetNextEventWrapperFinish(void)
{
	GR_EVENT evt;

	/* get the event and pass it to client*/
	/* this will never be GR_EVENT_TYPE_NONE*/
	GsCheckNextEvent(&evt);

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, (void *) &evt, sizeof(evt));
}

void GsCheckNextEventWrapper(void)
{
	GR_EVENT evt;

	GsCheckNextEvent(&evt);

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, (void *) &evt, sizeof(evt));
}

void GsPeekEventWrapper(void)
{
	GR_EVENT evt;
	GR_CHAR	ret;

	ret = GsPeekEvent(&evt);

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, (void *) &evt, sizeof(evt));

	GsWrite(current_fd, &ret, 1);
}

void GsFlushWrapper(void)
{
	GsPutCh(current_fd, GrRetOK);
}

void GsLineWrapper(void)
{
	GR_DRAW_ID id;
	GR_GC_ID gc;
	GR_COORD x1, y1, x2, y2;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &id, sizeof(id)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &x1, sizeof(x1)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &y1, sizeof(y1)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &x2, sizeof(x2)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &y2, sizeof(y2)))
		return;

	GsLine(id, gc, x1, y1, x2, y2);

	GsPutCh(current_fd, GrRetOK);	
}

void GsPointWrapper(void)
{
	GR_DRAW_ID id;
	GR_GC_ID gc;
	GR_COORD x, y;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &id, sizeof(id)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &x, sizeof(x)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &y, sizeof(y)))
		return;

	GsPoint(id, gc, x, y);

	GsPutCh(current_fd, GrRetOK);	

}

void GsRectWrapper(void)
{
	GR_DRAW_ID id;
	GR_GC_ID gc;
	GR_COORD x, y;
	GR_SIZE width, height;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &id, sizeof(id)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &x, sizeof(x)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &y, sizeof(y)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &width, sizeof(width)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &height, sizeof(height)))
		return;

	GsRect(id, gc, x, y, width, height);

	GsPutCh(current_fd, GrRetOK);	
}

void GsFillRectWrapper(void)
{
	GR_DRAW_ID id;
	GR_GC_ID gc;
	GR_COORD x, y;
	GR_SIZE width, height;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &id, sizeof(id)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &x, sizeof(x)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &y, sizeof(y)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &width, sizeof(width)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &height, sizeof(height)))
		return;

	GsFillRect(id, gc, x, y, width, height);

	GsPutCh(current_fd, GrRetOK);
}

/* FIXME: fails with pointtable size > 64k if sizeof(int) == 2*/
void GsPolyWrapper(void)
{
	GR_DRAW_ID id;
	GR_GC_ID gc;
	GR_COUNT count;
	GR_POINT *pointtable;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &id, sizeof(id)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &count, sizeof(count)))
		return;

	if(!(pointtable = malloc(count * sizeof(GR_POINT)))) {
		GsPutCh(current_fd, GrRetENoMem);
		return;
	}

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) pointtable, (count * sizeof(GR_POINT)))) {
		free(pointtable);
		return;
	}

	GsPoly(id, gc, count, pointtable);

	free(pointtable);

	GsPutCh(current_fd, GrRetOK);	
}

/* FIXME: fails with pointtable size > 64k if sizeof(int) == 2*/
void GsFillPolyWrapper(void)
{
	GR_DRAW_ID id;
	GR_GC_ID gc;
	GR_COUNT count;
	GR_POINT *pointtable;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &id, sizeof(id)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &count, sizeof(count)))
		return;

	if(!(pointtable = malloc(count * sizeof(GR_POINT)))) {
		GsPutCh(current_fd, GrRetENoMem);
		return;
	}

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) pointtable, (count * sizeof(GR_POINT)))) {
		free(pointtable);
		return;
	}

	GsFillPoly(id, gc, count, pointtable);

	free(pointtable);

	GsPutCh(current_fd, GrRetOK);	
}

void GsEllipseWrapper(void)
{
	GR_DRAW_ID id;
	GR_GC_ID gc;
	GR_COORD x, y;
	GR_SIZE rx, ry;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &id, sizeof(id)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &x, sizeof(x)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &y, sizeof(y)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &rx, sizeof(rx)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &ry, sizeof(ry)))
		return;

	GsEllipse(id, gc, x, y, rx, ry);

	GsPutCh(current_fd, GrRetOK);
}

void GsFillEllipseWrapper(void)
{
	GR_DRAW_ID id;
	GR_GC_ID gc;
	GR_COORD x, y;
	GR_SIZE rx, ry;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &id, sizeof(id)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &x, sizeof(x)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &y, sizeof(y)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &rx, sizeof(rx)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &ry, sizeof(ry)))
		return;

	GsFillEllipse(id, gc, x, y, rx, ry);

	GsPutCh(current_fd, GrRetOK);
}

void GsSetGCForegroundWrapper(void)
{
	GR_GC_ID gc;
	GR_COLOR foreground;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &foreground, sizeof(foreground)))
		return;

	GsSetGCForeground(gc, foreground);

	GsPutCh(current_fd, GrRetOK);
}

void GsSetGCBackgroundWrapper(void)
{
	GR_GC_ID gc;
	GR_COLOR background;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &background, sizeof(background)))
		return;

	GsSetGCBackground(gc, background);

	GsPutCh(current_fd, GrRetOK);
}

void GsSetGCUseBackgroundWrapper(void)
{
	GR_GC_ID gc;
	GR_BOOL use;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &use, sizeof(use)))
		return;

	GsSetGCUseBackground(gc, use);

	GsPutCh(current_fd, GrRetOK);
}

void GsSetGCModeWrapper(void)
{
	GR_GC_ID gc;
	GR_MODE mode;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &mode, sizeof(mode)))
		return;

	GsSetGCMode(gc, mode);

	GsPutCh(current_fd, GrRetOK);
}

void GsSetGCFontWrapper(void)
{
	GR_GC_ID gc;
	GR_FONT font;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &font, sizeof(font)))
		return;

	GsSetGCFont(gc, font);

	GsPutCh(current_fd, GrRetOK);
}

void GsGetGCTextSizeWrapper(void)
{
	GR_SIZE len, retwidth, retheight, retbase;
	GR_GC_ID gc;
	GR_CHAR *string;
	char strbuf[128];

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &len, sizeof(len)))
		return;

	if(len <= sizeof(strbuf))
		string = strbuf;
	else if(!(string = malloc(len))) {
		GsPutCh(current_fd, GrRetENoMem);
		return;
	}

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *)string, len)) {
		if(len > sizeof(strbuf))
			free(string);
		return;
	}

	GsGetGCTextSize(gc, string, len, &retwidth, &retheight, &retbase);

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, &retwidth, sizeof(retwidth));

	GsWrite(current_fd, &retheight, sizeof(retheight));

	GsWrite(current_fd, &retbase, sizeof(retbase));

	if(len > sizeof(strbuf))
		free(string);
}

/* FIXME: fails with size > 64k if sizeof(int) == 2*/
void GsReadAreaWrapper(void)
{
	GR_DRAW_ID id;
	GR_SIZE width, height;
	GR_COORD x, y;
	PIXELVAL *area;
	int size;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &id, sizeof(id)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &x, sizeof(x)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &y, sizeof(y)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &width, sizeof(width)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &height, sizeof(height)))
		return;

	/* FIXME: optimize for smaller pixelvals*/
	size = width * height * sizeof(PIXELVAL);

	if(!(area = malloc(size))) {
		GsPutCh(current_fd, GrRetENoMem);
		return;
	}

	GsReadArea(id, x, y, width, height, area);

	GsPutCh(current_fd, GrRetDataFollows);

	GsWrite(current_fd, area, size);

	free(area);
}

/* FIXME: fails with size > 64k if sizeof(int) == 2*/
void GsAreaWrapper(void)
{
	GR_DRAW_ID id;
	GR_GC_ID gc;
	GR_SIZE width, height;
	GR_COORD x, y;
	PIXELVAL *area;
	int size;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &id, sizeof(id)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &x, sizeof(x)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &y, sizeof(y)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &width, sizeof(width)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &height, sizeof(height)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	/* FIXME: optimize for smaller pixelvals*/
	size = width * height * sizeof(PIXELVAL);

	if(!(area = malloc(size))) {
		GsPutCh(current_fd, GrRetENoMem);
		return;
	}

	if(GsRead(current_fd, (void *) area, size))
		return;

	GsArea(id, gc, x, y, width, height, area);

	free(area);

	GsPutCh(current_fd, GrRetOK);
}

/* FIXME: fails with bitmapsize > 64k if sizeof(int) == 2*/
void GsBitmapWrapper(void)
{
	GR_DRAW_ID id;
	GR_GC_ID gc;
	GR_SIZE width, height;
	GR_COORD x, y;
	GR_BITMAP *bitmap;
	int bitmapsize;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &id, sizeof(id)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &x, sizeof(x)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &y, sizeof(y)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &width, sizeof(width)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &height, sizeof(height)))
		return;

	bitmapsize = GR_BITMAP_SIZE(width, height) * sizeof(GR_BITMAP);

	if(!(bitmap = malloc(bitmapsize))) {
		GsPutCh(current_fd, GrRetENoMem);
		return;
	}

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) bitmap, bitmapsize))
		return;

	GsBitmap(id, gc, x, y, width, height, bitmap);

	free(bitmap);

	GsPutCh(current_fd, GrRetOK);
}

void GsTextWrapper(void)
{
	GR_DRAW_ID id;
	GR_GC_ID gc;
	GR_COORD x, y;
	GR_COUNT count;
	GR_CHAR *str;
	char strbuf[128];

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &id, sizeof(id)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &gc, sizeof(gc)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &x, sizeof(x)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &y, sizeof(y)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &count, sizeof(count)))
		return;

	if(count <= sizeof(strbuf))
		str = strbuf;
	else if(!(str = malloc(count))) {
		GsPutCh(current_fd, GrRetENoMem);
		return;
	}

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) str, count)) {
		if(count > sizeof(strbuf))
			free(str);
		return;
	}

	GsText(id, gc, x, y, str, count);

	if(count > sizeof(strbuf))
		free(str);

	GsPutCh(current_fd, GrRetOK);
}

void GsSetCursorWrapper(void)
{
	GR_WINDOW_ID wid;
	GR_CURSOR cr;
	int bitmapsize;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &wid, sizeof(wid)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &cr.cursor.width, sizeof(cr.cursor.width)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &cr.cursor.height, sizeof(cr.cursor.height)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &cr.cursor.hotx, sizeof(cr.cursor.hotx)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &cr.cursor.hoty, sizeof(cr.cursor.hoty)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &cr.cursor.fgcolor, sizeof(cr.cursor.fgcolor)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) &cr.cursor.bgcolor, sizeof(cr.cursor.bgcolor)))
		return;

	bitmapsize = GR_BITMAP_SIZE(cr.cursor.width, cr.cursor.height)
		* sizeof(GR_BITMAP);

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) cr.cursor.image, bitmapsize))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, (void *) cr.cursor.mask, bitmapsize))
		return;

	GsSetCursor(wid, cr.cursor.width, cr.cursor.height,
		cr.cursor.hotx, cr.cursor.hoty, cr.cursor.fgcolor,
		cr.cursor.bgcolor, cr.cursor.image, cr.cursor.mask);

	GsPutCh(current_fd, GrRetOK);
}

void GsMoveCursorWrapper(void)
{
	GR_COORD x;
	GR_COORD y;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &x, sizeof(x)))
		return;

	GsPutCh(current_fd, GrRetSendData);

	if(GsRead(current_fd, &y, sizeof(y)))
		return;

	GsMoveCursor(x, y);

	GsPutCh(current_fd, GrRetOK);
}

/*
 * This is an array containing pointers to all of the above wrappers, in the same order as
 * the GrNum* #defines in nano-X.h. All the parser has to do is range check the function
 * number it gets from the client, and then call the relevant array member. The wrappers
 * do the work of reading the arguments and parsing them into the correct types and structures,
 * call the server functions themselves, then return any relevant data to the client.
 */

struct GrFunction {
	void (*func)(void);
	GR_FUNC_NAME name;
} GrFunctions[] = {
	{GsOpenWrapper, "GsOpen"},
	{GsCloseWrapper, "GsClose"},
	{GsGetScreenInfoWrapper, "GsGetScreenInfo"},
	{GsNewWindowWrapper, "GsNewWindow"},
	{GsNewInputWindowWrapper, "GsNewInputWindow"},
	{GsDestroyWindowWrapper, "GsDestroyWindow"},
	{GsNewGCWrapper, "GsNewGC"},
	{GsCopyGCWrapper, "GsCopyGC"},
	{GsGetGCInfoWrapper, "GsGetGCInfo"},
	{GsDestroyGCWrapper, "GsDestroyGC"},
	{GsMapWindowWrapper, "GsMapWindow"},
	{GsUnmapWindowWrapper, "GsUnmapWindow"},
	{GsRaiseWindowWrapper, "GsRaiseWindow"},
	{GsLowerWindowWrapper, "GsLowerWindow"},
	{GsMoveWindowWrapper, "GsMoveWindow"},
	{GsResizeWindowWrapper, "GsResizeWindow"},
	{GsGetWindowInfoWrapper, "GsGetWindowInfo"},
	{GsGetFontInfoWrapper, "GsGetFontInfo"},
	{GsSetFocusWrapper, "GsSetFocus"},
	{GsSetBorderColorWrapper, "GsSetBorderColor"},
	{GsClearWindowWrapper, "GsClearWindowWrapper"},
	{GsSelectEventsWrapper, "GsSelectEvents"},
	{GsGetNextEventWrapper, "GsGetNextEvent"},
	{GsCheckNextEventWrapper, "GsCheckNextEvent"},
	{GsPeekEventWrapper, "GsPeekEvent"},
	{GsFlushWrapper, "GsFlush"},
	{GsLineWrapper, "GsLine"},
	{GsPointWrapper, "GsPoint"},
	{GsRectWrapper, "GsRect"},
	{GsFillRectWrapper, "GsFillRect"},
	{GsPolyWrapper, "GsPoly"},
	{GsFillPolyWrapper, "GsFillPoly"},
	{GsEllipseWrapper, "GsEllipse"},
	{GsFillEllipseWrapper, "GsFillEllipse"},
	{GsSetGCForegroundWrapper, "GsSetGCForeground"},
	{GsSetGCBackgroundWrapper, "GsSetGCBackGround"},
	{GsSetGCUseBackgroundWrapper, "GsSetGCUseBackGround"},
	{GsSetGCModeWrapper, "GsSetGCMode"},
	{GsSetGCFontWrapper, "GsSetGCFont"},
	{GsGetGCTextSizeWrapper, "GsGetGCTextSize"},
	{GsReadAreaWrapper, "GsReadArea"},
	{GsAreaWrapper, "GsArea"},
	{GsBitmapWrapper, "GsBitmap"},
	{GsTextWrapper, "GsText"},
	{GsSetCursorWrapper, "GsSetCursor"},
	{GsMoveCursorWrapper, "GsMoveCursor"}
};

/*
 * This function is used to bind to the named socket which is used to
 * accept connections from the clients.
 */
int GsOpenSocket(void)
{
	struct stat s;
	struct sockaddr_un sckt;
#ifndef SUN_LEN
#define SUN_LEN(ptr)	((size_t) (((struct sockaddr_un *) 0)->sun_path) \
		      		+ strlen ((ptr)->sun_path))
#endif

	/* Check if the file already exists: */
	if(!stat(GR_NAMED_SOCKET, &s)) {
		/* FIXME: should try connecting to see if server is active */
		if(unlink(GR_NAMED_SOCKET))
			return -1;
	}

	/* Create the socket: */
	if((un_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;

	/* Bind a name to the socket: */
	sckt.sun_family = AF_UNIX;
	strncpy(sckt.sun_path, GR_NAMED_SOCKET, sizeof(sckt.sun_path));
	if(bind(un_sock, (struct sockaddr *) &sckt, SUN_LEN(&sckt)) < 0)
		return -1;

	/* Start listening on the socket: */
	if(listen(un_sock, 5) == -1)
		return -1;
	return 1;
}

void GsCloseSocket(void)
{
	if(un_sock != -1)
		close(un_sock);
	un_sock = -1;
	unlink(GR_NAMED_SOCKET);
}

/*
 * This function is used to accept a connnection from a client.
 */
void GsAcceptClient(void)
{
	int i;
	struct sockaddr_un sckt;
	size_t size = sizeof(sckt);

	if((i = accept(un_sock, (struct sockaddr *) &sckt, &size)) == -1) {
		printf("accept failed (%d)\n", errno);
		return;
	}
	GsAcceptClientFd(i);
}

/*
 * This function accepts a client ID, and searches through the linked list of clients,
 * returning a pointer to the relevant structure or NULL if not found.
 */
GR_CLIENT *GsFindClient(int fd)
{
	GR_CLIENT *client;

	client = root_client;

	while(client) {
		if(client->id == fd) return(client);
		client = client->next;
	}
	
	return 0;
}

/*
 * This is used to drop a client when it is detected that the connection to it
 * has been lost.
 */
void GsDropClient(int fd)
{
	GR_CLIENT *client;

	close(fd);	/* Close the socket */

	if((client = GsFindClient(fd))) { /* If it exists */
		if(client == root_client)
			root_client = client->next;
		if(client->prev) client->prev->next = client->next; /* Link the prev to the next */
		if(client->next) client->next->prev = client->prev; /* Link the next to the prev */
		free(client);	/* Free the structure */
	} else fprintf(stderr, "Error: trying to drop non-existent client %d.\n", fd);
}

/*
 * This is a wrapper to read() which handles error conditions, and
 * returns 0 for both error conditions and no data.
 */
int GsRead(int fd, void *buf, int c)
{
	int e, n;

	n = 0;

	while(n < c) {
		e = read(fd, ((char *)buf + n), (c - n));
		if(e <= 0) {
printf("GsRead: read failed %d %x %d: %d\r\n", e, ((char *)buf +n), c-n, errno);
			GsClose();
			return -1;
		}
		n += e;
	}

	return 0;
}

int GsPutCh(int fd, unsigned char c)
{
	return(GsWrite(fd, &c, 1));
}

/*
 * This is a wrapper to write().
 */
int GsWrite(int fd, void *buf, int c)
{
	int e, n;

	n = 0;

	while(n < c) {
		e = write(fd, ((char *) buf + n), (c - n));
		if(e <= 0) {
			GsClose();
			return -1;
		}
		n += e;
	}

	return 0;
}

/*
 * This function is used to parse and dispatch requests from the clients.
 */
void GsHandleClient(int fd)
{
	unsigned char c;

	current_fd = fd;

	if(GsRead(fd, &c, 1))
		return;

	if(c >= GrTotalNumCalls) {
		GsPutCh(fd, GrRetENoFunction);
	} else {
		curfunc = GrFunctions[c].name;
/*printf("HandleClient %s\r\n", curfunc);*/
		GrFunctions[c].func();
	}
}
