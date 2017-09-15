/*
 * Sets the maximum number of virtual consoles.
 *
 * NOTES:
 *
 * 1. If set to 0 then there is no backup store of display.
 *
 * 2. This value is included in several different source files, and is
 *    placed here to ensure the same value is used everywhere. Do not
 *    move it elsewhere.
 *
 * 3. Up to and including ELKS version 0.1.0-pre3 this was limited to
 *    a maximum of 3 as a result of the device node allocations for
 *    the serial ports. With effect from ELKS 0.1.0-pre4 this has been
 *    changed to a limit of 63 instead. However, the default needs to
 *    stay at 3 to prevent inadvertant conflicts arising.
 *
 */
/* This definition moved to include/linuxmt/ntty.h, to optimize space usage */
/*#define MAX_CONSOLES 3*/
