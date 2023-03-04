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

/* I/O address */
#define PC98_MOUSE_I_ADDR        0x7FD9
#define PC98_MOUSE_O_ADDR        0x7FDD
#define PC98_MOUSE_CONTROL_ADDR  0x7FDF

/* I/O bits */
#define PC98_O_HC   0x80 /* counter is hold and cleared */
#define PC98_O_X_L  0x00 /* X LSB 4bits are selected */
#define PC98_O_X_M  0x20 /* X MSB 4bits are selected */
#define PC98_O_Y_L  0x40 /* Y LSB 4bits are selected */
#define PC98_O_Y_M  0x60 /* Y MSB 4bits are selected */
#define PC98_O_C    0x93 /* countrol register value */

/* local data*/
static BUTTON       buttons;        /* current mouse buttons pressed */
static BUTTON       buttons_before; /* previous mouse buttons pressed */
static BUTTON       availbuttons;   /* which buttons are available */
static COORD        x_now;          /* current x counter value */
static COORD        y_now;          /* current y counter value */

static int      left;       /* because the button values change */
static int      right;      /* between mice, the buttons are redefined */

/* local routines*/
static int      MOU_Open(MOUSEDEVICE *pmd);
static void     MOU_Close(void);
static BUTTON   MOU_GetButtonInfo(void);
static void     MOU_GetDefaultAccel(int *pscale,int *pthresh);
static int      MOU_Read(COORD *dx, COORD *dy, COORD *dz, BUTTON *bptr);
static int      MOU_Poll(void);

MOUSEDEVICE mousedev = {
    MOU_Open,
    MOU_Close,
    MOU_GetButtonInfo,
    MOU_GetDefaultAccel,
    MOU_Read,
    MOU_Poll
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
    outportb(PC98_MOUSE_CONTROL_ADDR, PC98_O_C);

    if (!(port = getenv("MOUSE_PORT")))
        port = MOUSE_PORT;

    if (!strcmp(port, "none"))
        return -2;      /* no mouse */

    /* set button bits and parse procedure*/
    left = PC98_LEFT_BUTTON;
    right = PC98_RIGHT_BUTTON;

    /* initialize data*/
    availbuttons = LBUTTON | RBUTTON;
    buttons = 0;
    buttons_before = 0;

    return -3;
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
 * When a new state is read, the current buttons and x and y deltas
 * are returned. This routine does not block.
 */
static int
MOU_Read(COORD *dx, COORD *dy, COORD *dz, BUTTON *bptr)
{
    int b;

    /*
     * If the X, Y values are greater than 127,
     * then the delta would be negative.
     */
    if (x_now > 127)
        *dx = x_now - 256;
    else
        *dx = x_now;

    if (y_now > 127)
        *dy = y_now - 256;
    else
        *dy = y_now;

    *dz = 0;

    b = 0;
    if (buttons & left)
        b |= LBUTTON;
    if (buttons & right)
        b |= RBUTTON;
    *bptr = b;

    return 1;
}

static int
MOU_Poll(void)
{
    /*
     * The counter for x, y values are cleared every time.
     * If the x, y values are not equal to zero or
     * button states have changed then return 1.
     */
    outportb(PC98_MOUSE_O_ADDR, (PC98_O_HC | PC98_O_X_L)); // X LSB 4bits
    x_now = inportb(PC98_MOUSE_I_ADDR) & 0xF;

    outportb(PC98_MOUSE_O_ADDR, (PC98_O_HC | PC98_O_X_M)); // X MSB 4bits
    x_now += (inportb(PC98_MOUSE_I_ADDR) & 0xF) << 4;

    outportb(PC98_MOUSE_O_ADDR, (PC98_O_HC | PC98_O_Y_L)); // Y LSB 4bits
    y_now = inportb(PC98_MOUSE_I_ADDR) & 0xF;

    outportb(PC98_MOUSE_O_ADDR, (PC98_O_HC | PC98_O_Y_M)); // Y MSB 4bits
    y_now += (inportb(PC98_MOUSE_I_ADDR) & 0xF) << 4;

    buttons = inportb(PC98_MOUSE_I_ADDR) & 0xE0;

    outportb(PC98_MOUSE_O_ADDR, 0x00); // Clear HC

    if (x_now || y_now || (buttons != buttons_before)) {
        buttons_before = buttons;
        return 1;
    }
    else {
        return 0;
    }
}
