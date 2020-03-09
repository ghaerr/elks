/*
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Graphics server event routines for windows.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serv.h"

/*
 * Generate an error from a graphics function.
 * This creates a special event which describes the error.
 * Only one error event at a time can be saved for delivery to a client.
 * This is ok since there are usually lots of redundant errors generated
 * before the client can notice, errors occurs after the fact, and clients
 * can't do much about them except complain and die.  The error is saved
 * specially so that memory problems cannot occur.
 */
void GsError(GR_ERROR code, GR_ID id)
{
	GR_EVENT_ERROR	*ep;		/* event to describe error */

printf("GsError %d, %d\r\n", code, id);
	/*
	 * If there is already an outstanding error, then forget this one.
	 */
	if (curclient->errorevent.type == GR_EVENT_TYPE_ERROR)
		return;

	/*
	 * Save the error in a special location.
	 */
	ep = &curclient->errorevent;
	ep->type = GR_EVENT_TYPE_ERROR;
	ep->name[0] = 0;
	if (curfunc) {
		strncpy(ep->name, curfunc, sizeof(GR_FUNC_NAME));
		ep->name[sizeof(GR_FUNC_NAME)-1] = '\0';
	}
	ep->code = code;
	ep->id = id;
}

/*
 * Allocate an event to be passed back to the specified client.
 * The event is already chained onto the event queue, and only
 * needs filling out.  Returns NULL with an error generated if
 * the event cannot be allocated.
 */
GR_EVENT *GsAllocEvent(GR_CLIENT *client)
{
	GR_EVENT_LIST	*elp;		/* current element list */
	GR_CLIENT	*oldcurclient;	/* old current client */

	/*
	 * Get a new event structure from the free list, or else
	 * allocate it using malloc.
	 */
	elp = eventfree;
	if (elp)
		eventfree = elp->next;
	else {
		elp = (GR_EVENT_LIST *) malloc(sizeof(GR_EVENT_LIST));
		if (elp == NULL) {
			oldcurclient = curclient;
			curclient = client;
			GsError(GR_ERROR_MALLOC_FAILED, 0);
			curclient = oldcurclient;
			return NULL;
		}
	}

	/*
	 * Add the event to the end of the event list.
	 */
	if (client->eventhead)
		client->eventtail->next = elp;
	else
		client->eventhead = elp;
	client->eventtail = elp;
	elp->next = NULL;
	elp->event.type = GR_EVENT_TYPE_NONE;

	return &elp->event;
}

/*
 * Update mouse status and issue events on it if necessary.
 * This function doesn't block, but is normally only called when
 * there is known to be some data waiting to be read from the mouse.
 */
void GsCheckMouseEvent(void)
{
	COORD		rootx;		/* latest mouse x position */
	COORD		rooty;		/* latest mouse y position */
	BUTTON		newbuttons;	/* latest buttons */
	int		mousestatus;	/* latest mouse status */

	/* Read the latest mouse status: */
	mousestatus = GdReadMouse(&rootx, &rooty, &newbuttons);
	if (mousestatus < 0) {
		GsError(GR_ERROR_MOUSE_ERROR, 0);
		return;
	} else if (mousestatus) /* Deliver events as appropriate: */
		GsHandleMouseStatus(rootx, rooty, newbuttons);
}

/*
 * Update keyboard status and issue events on it if necessary.
 * This function doesn't block, but is normally only called when
 * there is known to be some data waiting to be read from the keyboard.
 */
void GsCheckKeyboardEvent(void)
{
	unsigned char	ch;		/* latest character */
	MODIFIER	modifiers;	/* latest modifiers */
	int		keystatus;	/* latest keyboard status */

	/* Read the latest keyboard status: */
	keystatus = GdReadKeyboard(&ch, &modifiers);
	if (keystatus < 0) {
		if (keystatus == -2)	/* special case for ESC pressed*/
			GsTerminate();
		GsError(GR_ERROR_KEYBOARD_ERROR, 0);
		return;
	} else if (keystatus) /* Deliver events as appropriate: */
		GsDeliverKeyboardEvent(GR_EVENT_TYPE_KEY_DOWN, ch, modifiers);
}

/*
 * Handle all mouse events.  These are mouse enter, mouse exit, mouse
 * motion, mouse position, button down, and button up.  This also moves
 * the cursor to the new mouse position and changes it shape if needed.
 */
void GsHandleMouseStatus(GR_COORD newx, GR_COORD newy, BUTTON newbuttons)
{
	BUTTON	changebuttons;	/* buttons that have changed */
	unsigned char	ch;		/* latest character */
	MODIFIER	modifiers;	/* latest modifiers */

	GdReadKeyboard(&ch, &modifiers); /* Read the modifiers */

	/*
	 * First, if the mouse has moved, then position the cursor to the
	 * new location, which will send mouse enter, mouse exit, focus in,
	 * and focus out events if needed.  Check here for mouse motion and
	 * mouse position events.  Flush the device queue to make sure the
	 * new cursor location is quickly seen by the user.
	 */
	if ((newx != cursorx) || (newy != cursory)) {
		GsMoveCursor(newx, newy);
		GsFlush();
		GsDeliverMotionEvent(GR_EVENT_TYPE_MOUSE_MOTION,
			newbuttons, modifiers);
		GsDeliverMotionEvent(GR_EVENT_TYPE_MOUSE_POSITION,
			newbuttons, modifiers);
	}

	/*
	 * Next, generate a button up event if any buttons have been released.
	 */
	changebuttons = (curbuttons & ~newbuttons);
	if (changebuttons) {
		GsDeliverButtonEvent(GR_EVENT_TYPE_BUTTON_UP,
			newbuttons, changebuttons, modifiers);
	}

	/*
	 * Finally, generate a button down event if any buttons have been
	 * pressed.
	 */
	changebuttons = (~curbuttons & newbuttons);
	if (changebuttons) {
		GsDeliverButtonEvent(GR_EVENT_TYPE_BUTTON_DOWN,
			newbuttons, changebuttons, modifiers);
	}

	curbuttons = newbuttons;
}

/*
 * Deliver a mouse button event to the clients which have selected for it.
 * Each client can only be delivered one instance of the event.  The window
 * the event is delivered for is either the smallest one containing the
 * mouse coordinates, or else one of its direct ancestors.  The lowest
 * window in that tree which has enabled for the event gets it.  This scan
 * is done independently for each client.  If a window with the correct
 * noprop mask is reached, or if no window selects for the event, then the
 * event is discarded for that client.  Special case: for the first client
 * that is enabled for both button down and button up events in a window,
 * then the pointer is implicitly grabbed by that window when a button is
 * pressed down in that window.  The grabbing remains until all buttons are
 * released.  While the pointer is grabbed, no other clients or windows can
 * receive button down or up events.
 */
void GsDeliverButtonEvent(GR_EVENT_TYPE type, BUTTON buttons, BUTTON changebuttons,
			MODIFIER modifiers)
{
	GR_EVENT_BUTTON	*ep;		/* mouse button event */
	GR_WINDOW	*wp;		/* current window */
	GR_EVENT_CLIENT	*ecp;		/* current event client */
	GR_CLIENT	*client;	/* current client */
	GR_WINDOW_ID	subwid;		/* subwindow id event is for */
	GR_EVENT_MASK	eventmask;	/* event mask */
	GR_EVENT_MASK	tempmask;	/* to get around compiler bug */

	eventmask = GR_EVENTMASK(type);
	if (eventmask == 0)
		return;

	/*
	 * If the pointer is implicitly grabbed, then the only window
	 * which can receive button events is that window.  Otherwise
	 * the window the pointer is in gets the events.  Determine the
	 * subwindow by seeing if it is a child of the grabbed button.
	 */
	wp = mousewp;
	subwid = wp->id;

	if (grabbuttonwp) {
		while ((wp != rootwp) && (wp != grabbuttonwp))
			wp = wp->parent;
		if (wp != grabbuttonwp)
			subwid = grabbuttonwp->id;
		wp = grabbuttonwp;
	}

	for (;;) {
		for (ecp = wp->eventclients; ecp; ecp = ecp->next) {
			if ((ecp->eventmask & eventmask) == 0)
				continue;

			client = ecp->client;

			/*
			 * If this is a button down, the buttons are not
			 * yet grabbed, and this client is enabled for both
			 * button down and button up events, then implicitly
			 * grab the window for him.
			 */
			if ((type == GR_EVENT_TYPE_BUTTON_DOWN)
				&& (grabbuttonwp == NULL))
			{
				tempmask = GR_EVENT_MASK_BUTTON_UP;
				if (ecp->eventmask & tempmask)
					grabbuttonwp = wp;
			}

			ep = (GR_EVENT_BUTTON *) GsAllocEvent(client);
			if (ep == NULL)
				continue;

			ep->type = type;
			ep->wid = wp->id;
			ep->subwid = subwid;
			ep->rootx = cursorx;
			ep->rooty = cursory;
			ep->x = cursorx - wp->x;
			ep->y = cursory - wp->y;
			ep->buttons = buttons;
			ep->changebuttons = changebuttons;
			ep->modifiers = modifiers;
		}

		/*
		 * Events do not propagate if the window was grabbed.
		 * Also release the grab if the buttons are now all released,
		 * which can cause various events.
		 */
		if (grabbuttonwp) {
			if (buttons == 0) {
				grabbuttonwp = NULL;
				GsMoveCursor(cursorx, cursory);
				GsFlush();
			}
			return;
		}

		if ((wp == rootwp) || (wp->nopropmask & eventmask))
			return;

		wp = wp->parent;
	}
}

/*
 * Deliver a mouse motion event to the clients which have selected for it.
 * Each client can only be delivered one instance of the event.  The window
 * the event is delivered for is either the smallest one containing the
 * mouse coordinates, or else one of its direct ancestors.  The lowest
 * window in that tree which has enabled for the event gets it.  This scan
 * is done independently for each client.  If a window with the correct
 * noprop mask is reached, or if no window selects for the event, then the
 * event is discarded for that client.  Special case: If the event type is
 * GR_EVENT_TYPE_MOUSE_POSITION, then only the last such event is queued for
 * any single client to reduce events.  If the mouse is implicitly grabbed,
 * then only the grabbing window receives the events, and continues to do
 * so even if the mouse is currently outside of the grabbing window.
 */
void GsDeliverMotionEvent(GR_EVENT_TYPE type, BUTTON buttons, MODIFIER modifiers)
{
	GR_EVENT_MOUSE	*ep;		/* mouse motion event */
	GR_WINDOW	*wp;		/* current window */
	GR_EVENT_CLIENT	*ecp;		/* current event client */
	GR_CLIENT	*client;	/* current client */
	GR_WINDOW_ID	subwid;		/* subwindow id event is for */
	GR_EVENT_MASK	eventmask;	/* event mask */

	eventmask = GR_EVENTMASK(type);
	if (eventmask == 0)
		return;

	wp = mousewp;
	subwid = wp->id;

	if (grabbuttonwp) {
		while ((wp != rootwp) && (wp != grabbuttonwp))
			wp = wp->parent;
		if (wp != grabbuttonwp)
			subwid = grabbuttonwp->id;
		wp = grabbuttonwp;
	}

	for (;;) {
		for (ecp = wp->eventclients; ecp; ecp = ecp->next) {
			if ((ecp->eventmask & eventmask) == 0)
				continue;

			client = ecp->client;



			/*
			 * If the event is for just the latest position,
			 * then search the event queue for an existing
			 * event of this type (if any), and free it.
			 */
			if (type == GR_EVENT_TYPE_MOUSE_POSITION)
				GsFreePositionEvent(client, wp->id, subwid);

			ep = (GR_EVENT_MOUSE *) GsAllocEvent(client);
			if (ep == NULL)
				continue;

			ep->type = type;
			ep->wid = wp->id;
			ep->subwid = subwid;
			ep->rootx = cursorx;
			ep->rooty = cursory;
			ep->x = cursorx - wp->x;
			ep->y = cursory - wp->y;
			ep->buttons = buttons;
			ep->modifiers = modifiers;
		}

		if ((wp == rootwp) || grabbuttonwp ||
			(wp->nopropmask & eventmask))
				return;

		wp = wp->parent;
	}
}

/*
 * Deliver a keyboard event to one of the clients which have selected for it.
 * Only the first client found gets the event (no duplicates are sent).  The
 * window the event is delivered to is either the smallest one containing
 * the mouse coordinates, or else one of its direct ancestors (if such a
 * window is a descendant of the focus window), or else just the focus window.
 * The lowest window in that tree which has enabled for the event gets it.
 * If a window with the correct noprop mask is reached, or if no window selects
 * for the event, then the event is discarded.
 */
void GsDeliverKeyboardEvent(GR_EVENT_TYPE type, GR_CHAR ch, MODIFIER modifiers)
{
	GR_EVENT_KEYSTROKE	*ep;		/* keystroke event */
	GR_WINDOW		*wp;		/* current window */
	GR_WINDOW		*tempwp;	/* temporary window pointer */
	GR_EVENT_CLIENT		*ecp;		/* current event client */
	GR_WINDOW_ID		subwid;		/* subwindow id event is for */
	GR_EVENT_MASK		eventmask;	/* event mask */

	eventmask = GR_EVENTMASK(type);
	if (eventmask == 0)
		return;

	wp = mousewp;
	subwid = wp->id;

	/*
	 * See if the actual window the pointer is in is a descendant of
	 * the focus window.  If not, then ignore it, and force the input
	 * into the focus window itself.
	 */
	tempwp = wp;
	while ((tempwp != focuswp) && (tempwp != rootwp))
		tempwp = tempwp->parent;

	if (tempwp != focuswp) {
		wp = focuswp;
		subwid = wp->id;
	}

	/*
	 * Now walk upwards looking for the first window which will accept
	 * the keyboard event.  However, do not go beyond the focus window,
	 * and only give the event to one client.
	 */
	for (;;) {
		for (ecp = wp->eventclients; ecp; ecp = ecp->next) {
			if ((ecp->eventmask & eventmask) == 0)
				continue;

			ep = (GR_EVENT_KEYSTROKE *) GsAllocEvent(ecp->client);
			if (ep == NULL)
				return;

			ep->type = type;
			ep->wid = wp->id;
			ep->subwid = subwid;
			ep->rootx = cursorx;
			ep->rooty = cursory;
			ep->x = cursorx - wp->x;
			ep->y = cursory - wp->y;
			ep->buttons = curbuttons;
			ep->modifiers = modifiers;
			ep->ch = ch;
			return;			/* only one client gets it */
		}

		if ((wp == rootwp) || (wp == focuswp) ||
			(wp->nopropmask & eventmask))
				return;

		wp = wp->parent;
	}
}

/*
 * Try to deliver a exposure event to the clients which have selected for it.
 * This does not send exposure events for unmapped or input-only windows.
 * Exposure events do not propagate upwards.
 */
void GsDeliverExposureEvent(GR_WINDOW *wp, GR_COORD x, GR_COORD y, GR_SIZE width,
				GR_SIZE height)
{
	GR_EVENT_EXPOSURE	*ep;		/* exposure event */
	GR_EVENT_CLIENT		*ecp;		/* current event client */

	if (wp->unmapcount || !wp->output)
		return;


	for (ecp = wp->eventclients; ecp; ecp = ecp->next) {
		if ((ecp->eventmask & GR_EVENT_MASK_EXPOSURE) == 0)
			continue;

		ep = (GR_EVENT_EXPOSURE *) GsAllocEvent(ecp->client);
		if (ep == NULL)
			continue;

		ep->type = GR_EVENT_TYPE_EXPOSURE;
		ep->wid = wp->id;
		ep->x = x;
		ep->y = y;
		ep->width = width;
		ep->height = height;
	}
}

/*
 * Try to deliver a general event such as focus in, focus out, mouse enter,
 * or mouse exit to the clients which have selected for it.  These events
 * only have the window id as data, and do not propagate upwards.
 */
void GsDeliverGeneralEvent(GR_WINDOW *wp, GR_EVENT_TYPE type)
{
	GR_EVENT_GENERAL	*gp;		/* general event */
	GR_EVENT_CLIENT		*ecp;		/* current event client */
	GR_EVENT_MASK		eventmask;	/* event mask */

	eventmask = GR_EVENTMASK(type);
	if (eventmask == 0)
		return;

	for (ecp = wp->eventclients; ecp; ecp = ecp->next) {
		if ((ecp->eventmask & eventmask) == 0)
			continue;

		gp = (GR_EVENT_GENERAL *) GsAllocEvent(ecp->client);
		if (gp == NULL)
			continue;

		gp->type = type;
		gp->wid = wp->id;
	}
}

/*
 * Search for a matching mouse position event in the specified client's
 * event queue, and remove it.  This is used to prevent multiple position
 * events from being delivered, thus providing a more efficient rubber-
 * banding effect than if the mouse motion events were all sent.
 */
void GsFreePositionEvent(GR_CLIENT *client, GR_WINDOW_ID wid, GR_WINDOW_ID subwid)
{
	GR_EVENT_LIST	*elp;		/* current element list */
	GR_EVENT_LIST	*prevelp;	/* previous element list */

	prevelp = NULL;
	for (elp = client->eventhead; elp; prevelp = elp, elp = elp->next) {
		if (elp->event.type != GR_EVENT_TYPE_MOUSE_POSITION)
			continue;
		if (elp->event.mouse.wid != wid)
			continue;
		if (elp->event.mouse.subwid != subwid)
			continue;

		/*
		 * Found one, remove it and put it back on the free list.
		 */
		if (prevelp)
			prevelp->next = elp->next;
		else
			client->eventhead = elp->next;
		if (client->eventtail == elp)
			client->eventtail = prevelp;

		elp->next = eventfree;
		eventfree = elp;
		return;
	}
}
