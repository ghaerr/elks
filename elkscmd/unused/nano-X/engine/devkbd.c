/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Device-independent keyboard routines
 */
#include "device.h"

/*
 * Open the keyboard.
 */
int
GdOpenKeyboard(void)
{
	return kbddev.Open(&kbddev);
}

/*
 * Close the keyboard.
 */
void
GdCloseKeyboard(void)
{
	kbddev.Close();
}

/*
 * Return the possible modifiers for the keyboard.
 */
void
GdGetModifierInfo(MODIFIER *modifiers)
{
	kbddev.GetModifierInfo(modifiers);
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the mode keys (ALT, SHIFT, CTRL).  Returns -1 on error, 0 if no data
 * is ready, and 1 if data was read.  This is a non-blocking call.
 * Returns -2 if ESC pressed.
 */
int
GdReadKeyboard(UCHAR *buf, MODIFIER *modifiers)
{
	return kbddev.Read(buf, modifiers);
}
