/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Keyboard Driver, PC bios version
 */
#include "../device.h"

static int  KBD_Open(KBDDEVICE *pkd);
static void KBD_Close(void);
static void KBD_GetModifierInfo(MODIFIER *modifiers);
static int  KBD_Read(UCHAR *buf, MODIFIER *modifiers);
static int	KBD_Poll(void);

/* external routines in asmkbd.s*/
extern int kbpoll(void);
extern int kbread(void);
extern int kbflags(void);

KBDDEVICE kbddev = {
	KBD_Open,
	KBD_Close,
	KBD_GetModifierInfo,
	KBD_Read,
	KBD_Poll
};

/*
 * Open the keyboard.
 */
static int
KBD_Open(KBDDEVICE *pkd)
{
	return 1;

}

/*
 * Close the keyboard.
 */
static void
KBD_Close(void)
{
}

/*
 * Return the possible modifiers for the keyboard.
 */
static  void
KBD_GetModifierInfo(MODIFIER *modifiers)
{
	*modifiers = 0;			/* no modifiers available */
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the mode keys (ALT, SHIFT, CTRL).  Returns -1 on error, 0 if no data
 * is ready, and 1 if data was read.  This is a non-blocking call.
 */
static int
KBD_Read(UCHAR *buf, MODIFIER *modifiers)
{
	/* wait until a char is ready*/
	if(!kbpoll())
		return 0;

	/* read keyboard shift status*/
	*modifiers = kbflags();

	/* read keyboard character*/
	*buf = kbread();

	if(*buf == 0x1b)			/* special case ESC*/
		return -2;
	return 1;
}

static int
KBD_Poll(void)
{
	return kbpoll();
}
