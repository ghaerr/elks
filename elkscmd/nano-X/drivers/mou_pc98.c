/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * PC-98 Bus Mouse Driver
 * This driver is created and modified, based on mou_ser.c
 * T. Yamada 2023
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../device.h"

extern int  inportb(int port);
extern void outportb(int port,unsigned char data);

#define SCALE       3   /* default scaling factor for acceleration */
#define THRESH      5   /* default threshhold for acceleration */

/* default settings*/
#define MOUSE_PORT  "use"   /* default */

/* values in the bytes returned by the mouse for the buttons*/
#define PC98_LEFT_BUTTON    0x80
#define PC98_RIGHT_BUTTON   0x20

/* local data*/
static BUTTON       buttons;    /* current mouse buttons pressed*/
static BUTTON       availbuttons;   /* which buttons are available */
static COORD        x_before = 0;
static COORD        y_before = 0;

static int      left;       /* because the button values change */
static int      right;      /* between mice, the buttons are redefined */

/* local routines*/
static int      MOU_Open(MOUSEDEVICE *pmd);
static void     MOU_Close(void);
static BUTTON   MOU_GetButtonInfo(void);
static void MOU_GetDefaultAccel(int *pscale,int *pthresh);
static int      MOU_Read(COORD *dx, COORD *dy, COORD *dz, BUTTON *bptr);

MOUSEDEVICE mousedev = {
    MOU_Open,
    MOU_Close,
    MOU_GetButtonInfo,
    MOU_GetDefaultAccel,
    MOU_Read,
    NULL
};

/*
 * Open up the mouse device.
 */
static int
MOU_Open(MOUSEDEVICE *pmd)
{
    char    *port;

    /* Control Register for 8255A */
    /* Port A input, Port B input, Port C MSB output, Port C LSB input */
    outportb(0x7FDF, 0x93);

    if( !(port = getenv("MOUSE_PORT")))
        port = MOUSE_PORT;

    if (!strcmp(port, "none")) {
        return -2;      /* no mouse */
    }

    /* set button bits and parse procedure*/
    left = PC98_LEFT_BUTTON;
    right = PC98_RIGHT_BUTTON;

    /* initialize data*/
    availbuttons = left | right;
    buttons = 0;

    return 0;
}

/*
 * Close the mouse device.
 */
static void
MOU_Close(void)
{
}

/*
 * Get mouse buttons supported
 */
static BUTTON
MOU_GetButtonInfo(void)
{
    return availbuttons;
}

/*
 * Get default mouse acceleration settings
 */
static void
MOU_GetDefaultAccel(int *pscale,int *pthresh)
{
    *pscale = SCALE;
    *pthresh = THRESH;
}

/*
 * Attempt to read bytes from the mouse and interpret them.
 * Returns 1 for the new state read.
 * When a new state is read, the current buttons and x and y deltas
 * are returned.  This routine does not block.
 */
static int
MOU_Read(COORD *dx, COORD *dy, COORD *dz, BUTTON *bptr)
{
    int b;
    COORD x_now;
    COORD y_now;

    /*
     * If the X, Y values are smaller than the values read before,
     * then the delta would be negative.
     * If the delta are greater than 127,
     * then we assume the counter rolled backward.
     */
    outportb(0x7FDD, 0x80); // X LSB 4bits
    x_now = inportb(0x7FD9) & 0xF;
    outportb(0x7FDD, 0xA0); // X MSB 4bits
    x_now += (inportb(0x7FD9) & 0xF) << 4;
    outportb(0x7FDD, 0xC0); // Y LSB 4bits
    y_now = inportb(0x7FD9) & 0xF;
    outportb(0x7FDD, 0xE0); // Y MSB 4bits
    y_now += (inportb(0x7FD9) & 0xF) << 4;
    buttons = inportb(0x7FD9) & 0xE0;

    if (x_now - x_before > 127)
        *dx = x_now - x_before - 256;
    else
        *dx = x_now - x_before;
    
    if (y_now - y_before > 127)
        *dy = y_now - y_before - 256;
    else
        *dy = y_now - y_before;

    *dz = 0;

    x_before = x_now;
    y_before = y_now;

    b = 0;
    if(buttons & left)
        b |= LBUTTON;
    if(buttons & right)
        b |= RBUTTON;
    *bptr = b;

    outportb(0x7FDD, 0x00); // Clear HS

    return 1;
}
