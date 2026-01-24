/*
 * ELKS user mode mouse driver and test program.
 *  Use -DDRIVER=1 for elkscmd/gui (paint) mouse driver.
 *
 * Opens a serial port directly, and interprets serial data.
 * Microsoft, PC/Logitech and PS/2 mice are supported.
 *
 * Mouse driver from Microwindows
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>

#ifdef DRIVER
#include "event.h"
#else
/* default mouse button values */
#define BUTTON_L            0x01    /* left button*/
#define BUTTON_R            0x02    /* right button*/
#define BUTTON_M            0x10    /* middle*/
#define BUTTON_SCROLLUP     0x20    /* wheel up*/
#define BUTTON_SCROLLDN     0x40    /* wheel down*/
#endif

/* configurable options */
#define MOUSE_PORT  "/dev/ttyS0"    /* default port unless MOUSE_PORT= env specified */
#define MOUSE_QEMU  "/dev/ttyS1"    /* default port on QEMU */
#define MOUSE_PS2   "/dev/psaux"    /* default port for PS/2 mouse */
#define MOUSE_TYPE          "ms"    /* default mouse is microsoft mouse */

#define USE_MOUSE_ACCEL     0       /* =1 to use mouse acceleration */
#define SCALE               2       /* default scaling factor for acceleration */
#define THRESH              5       /* default threshhold for acceleration */

/* states for the mouse */
#define IDLE                0       /* start of byte sequence */
#define XSET                1       /* setting x delta */
#define YSET                2       /* setting y delta */
#define XADD                3       /* adjusting x delta */
#define YADD                4       /* adjusting y delta */

/* pc and ms button codes*/
#define PC_LEFT_BUTTON      4
#define PC_MIDDLE_BUTTON    2
#define PC_RIGHT_BUTTON     1
#define MS_LEFT_BUTTON      2
#define MS_RIGHT_BUTTON     1

/* ps/2 button codes */
#define PS2_LEFT_BUTTON     1
#define PS2_RIGHT_BUTTON    2

/* Bit fields in the bytes sent by the mouse */
#define TOP_FIVE_BITS       0xf8
#define BOTTOM_THREE_BITS   0x07
#define TOP_BIT             0x80
#define SIXTH_BIT           0x40
#define BOTTOM_TWO_BITS     0x03
#define THIRD_FOURTH_BITS   0x0c
#define BOTTOM_SIX_BITS     0x3f
#define PS2_CTRL_BYTE       0x08

int     mouse_fd = -1;      /* file descriptor for mouse */

/* local data*/
static int      rawmode;    /* show raw mouse data */
static int      state;      /* IDLE, XSET, ... */
static int      buttons;    /* current mouse buttons pressed*/
static int      availbuttons;   /* which buttons are available */
static int      xd;         /* change in x */
static int      yd;         /* change in y */

static int      left;       /* because the button values change */
static int      middle;     /* between mice, the buttons are */
static int      right;      /* redefined */

static unsigned char    *bp;/* buffer pointer */
static int      nbytes;     /* number of bytes left */
static int      (*parse)(); /* parse routine */
static int      parsePC(int);       /* routine to interpret PC mouse */
static int      parseMS(int);       /* routine to interpret MS mouse */
static int      parsePS2(int);      /* routine to interpret MS mouse */

/*
 * NOTE: max_bytes can't be larger than 1 mouse read packet or select() can fail,
 * as mouse driver would be storing unprocessed data not seen by select.
 */
#define PC_MAX_BYTES   5            /* max read() w/o storing excess mouse data */
#define MS_MAX_BYTES   3            /* max read() w/o storing excess mouse data */
#define PS2_MAX_BYTES  3            /* max read() w/o storing excess mouse data */
#define MAX_BYTES      5
int max_bytes = MS_MAX_BYTES;

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
int open_mouse(void)
{
    char *port, *type;
    struct termios termios;

    if( !(type = getenv("MOUSE_TYPE")))
        type = MOUSE_TYPE;

    /* set button bits and parse procedure*/
    if(!strcmp(type, "pc")) {
        /* pc or logitech mouse*/
        left = PC_LEFT_BUTTON;
        middle = PC_MIDDLE_BUTTON;
        right = PC_RIGHT_BUTTON;
        parse = parsePC;
        max_bytes = PC_MAX_BYTES;
    } else if (strcmp(type, "ms") == 0) {
        /* microsoft mouse*/
        left = MS_LEFT_BUTTON;
        right = MS_RIGHT_BUTTON;
        middle = 0;
        parse = parseMS;
        max_bytes = MS_MAX_BYTES;
    } else if (strcmp(type, "ps2") == 0) {
        printf("got PS2 %s\n", type);
        /* PS/2 mouse*/
        left = PS2_LEFT_BUTTON;
        right = PS2_RIGHT_BUTTON;
        middle = 0;
        parse = parsePS2;
        max_bytes = PS2_MAX_BYTES;
    } else {
        printf("Unknown mouse type: %s\n", type);
        return -1;
    }

    /* open mouse port*/
    if (parse == parsePS2)
        port = MOUSE_PS2;
    else if (!(port = getenv("MOUSE_PORT")))
        port = getenv("QEMU")? MOUSE_QEMU: MOUSE_PORT;
    printf("Opening mouse on %s\n", port);
    mouse_fd = open(port, O_RDWR | O_EXCL | O_NOCTTY | O_NONBLOCK);
    if (mouse_fd < 0) {
        printf("Can't open mouse %s, error %d. Try export MOUSE_PORT=/dev/ttyS1\n",
            port, errno);
        return -1;
    }

    /* set rawmode serial port using termios*/
    if (tcgetattr(mouse_fd, &termios) < 0) {
        printf("Can't get termio on %s, error %d\n", port, errno);
        close(mouse_fd);
        return -1;
    }

    if(cfgetispeed(&termios) != B1200)
        cfsetispeed(&termios, (speed_t)B1200);

    termios.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    //termios.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT | IGNBRK);
    //termios.c_cflag &= ~(CSIZE | PARENB);
    termios.c_cflag |= CS8;
    termios.c_cc[VMIN] = 0;
    termios.c_cc[VTIME] = 0;
    if(tcsetattr(mouse_fd, TCSAFLUSH, &termios) < 0) {
        printf("Can't set termio on %s, error %d\n", port, errno);
        close(mouse_fd);
        return -1;
    }

#if UNUSED /* MOUSE_PS2 */
    /* sequence to mouse device to send ImPS/2 events*/
    static const unsigned char imps2[] = { 0xf3, 200, 0xf3, 100, 0xf3, 80 };
    char buf[4];

    /* try to switch the mouse to ImPS/2 protocol*/
    if (write(mouse_fd, imps2, sizeof(imps2)) != sizeof(imps2))
        /*printf("Can't switch to ImPS/2 protocol\n")*/;
    if (read(mouse_fd, buf, 4) != 1 || buf[0] != 0xF4)
        /*printf("Failed to switch to ImPS/2 protocol.\n")*/;
#endif

    /* initialize data*/
    availbuttons = left | middle | right;
    state = IDLE;
    nbytes = 0;
    buttons = 0;
    xd = 0;
    yd = 0;

    return mouse_fd;
}

void close_mouse(void)
{
    if (mouse_fd >= 0)
        close(mouse_fd);
    mouse_fd = -1;
}

#if USE_MOUSE_ACCEL
static void filter_relative(int *xpos, int *ypos, int x, int y)
{
    int sign = 1;

    if (x < 0) {
        sign = -1;
        x = -x;
    }

    if (x > THRESH)
        x = THRESH + (x - THRESH) * SCALE;
    x *= sign;

    sign = 1;

    if (y < 0) {
        sign = -1;
        y = -y;
    }

    if (y > THRESH)
        y = THRESH + (y - THRESH) * SCALE;
    y *= sign;

    *xpos = x;
    *ypos = y;
}
#endif

#if UNUSED  /* MOUSE_PS2 */
/* IntelliMouse PS/2 protocol uses four byte reports
 * (PS/2 protocol omits last byte):
 *      Bit   7     6     5     4     3     2     1     0
 * --------+-----+-----+-----+-----+-----+-----+-----+-----
 *  Byte 0 |  0     0   Neg-Y Neg-X   1    Mid  Right Left
 *  Byte 1 |  X     X     X     X     X     X     X     X
 *  Byte 2 |  Y     Y     Y     Y     Y     Y     Y     Y
 *  Byte 3 |  W     W     W     W     W     W     W     W
 *
 * XXXXXXXX, YYYYYYYY, and WWWWWWWW are 8-bit two's complement values
 * indicating changes in x-coordinate, y-coordinate, and scroll wheel.
 * That is, 0 = no change, 1..127 = positive change +1 to +127,
 * and 129..255 = negative change -127 to -1.
 *
 * Left, Right, and Mid are the three button states, 1 if being depressed.
 * Neg-X and Neg-Y are set if XXXXXXXX and YYYYYYYY are negative, respectively.
 */
int read_mouse(int *dx, int *dy, int *dw, int *bp)
{
    int n, x, y, w, left, middle, right, button;
    unsigned char data[4];

    n = read(mouse_fd, data, sizeof(data));
    if (n != 3 && n != 4)
        return 0;

    button = 0;
    left = data[0] & 0x1;
    right = data[0] & 0x2;
    middle = data[0] & 0x4;

    if (left)   button |= BUTTON_L;
    if (middle) button |= BUTTON_M;
    if (right)  button |= BUTTON_R;

    x =   (signed char) data[1];
    y = - (signed char) data[2];  /* y axis flipped between conventions */
    if (n == 4) {
        w = (signed char) data[3];
        if (w > 0)
            button |= BUTTON_SCROLLUP;
        if (w < 0)
            button |= BUTTON_SCROLLDN;
    }
    *dx = x;
    *dy = y;
    *dw = w;
    *bp = button;
    return 1;
}
#endif

/*
 * Attempt to read bytes from the mouse and interpret them.
 * Returns -1 on error, 0 if either no bytes were read or not enough
 * was read for a complete state, or 1 if the new state was read.
 * When a new state is read, the current buttons and x and y deltas
 * are returned.  This routine does not block.
 */
int read_mouse(int *dx, int *dy, int *dz, int *bptr)
{
    int b;
    static unsigned char buffer[MAX_BYTES];

    /*
     * If there are no more bytes left, then read some more,
     * waiting for them to arrive.  On a signal or a non-blocking
     * error, return saying there is no new state available yet.
     */
    if (nbytes <= 0) {
        bp = buffer;
        nbytes = read(mouse_fd, bp, max_bytes);
        if (nbytes < 0) {
            if (errno == EINTR || errno == EAGAIN)
                return 0;
            return -1;
        }
    }
    if (rawmode) {
        while (nbytes-- > 0)
            printf("%02x ", *bp++);
        return 0;
    }

    /*
     * Loop over all the bytes read in the buffer, parsing them.
     * When a complete state has been read, return the results,
     * leaving further bytes in the buffer for later calls.
     */
    while (nbytes-- > 0) {
        if ((*parse)((int) *bp++)) {
#if USE_MOUSE_ACCEL
            if (parse == parseMS)   /* filter works on MS mouse only */
                filter_relative(&xd, &yd, xd, yd);
#endif
            *dx = xd;
            *dy = yd;
            *dz = 0;
            b = 0;
            if(buttons & left)
                b |= BUTTON_L;
            if(buttons & right)
                b |= BUTTON_R;
            if(buttons & middle)
                b |= BUTTON_M;
            *bptr = b;
            return 1;
        }
    }

    return 0;
}

/*
 * Input routine for PC mouse.
 * Returns nonzero when a new mouse state has been completed.
 */
static int parsePC(int byte)
{
    int sign;           /* sign of movement */

    switch (state) {
        case IDLE:
            if ((byte & TOP_FIVE_BITS) == TOP_BIT) {
                buttons = ~byte & BOTTOM_THREE_BITS;
                state = XSET;
            }
            break;

        case XSET:
            sign = 1;
            if (byte > 127) {
                byte = 256 - byte;
                sign = -1;
            }
            xd = byte * sign;
            state = YSET;
            break;

        case YSET:
            sign = 1;
            if (byte > 127) {
                byte = 256 - byte;
                sign = -1;
            }
            yd = -byte * sign;
            state = XADD;
            break;

        case XADD:
            sign = 1;
            if (byte > 127) {
                byte = 256 - byte;
                sign = -1;
            }
            xd += byte * sign;
            state = YADD;
            break;

        case YADD:
            sign = 1;
            if (byte > 127) {
                byte = 256 - byte;
                sign = -1;
            }
            yd -= byte * sign;
            state = IDLE;
            return 1;
    }
    return 0;
}

/*
 * Input routine for Microsoft mouse.
 * Returns nonzero when a new mouse state has been completed.
 */
static int parseMS(int byte)
{
    switch (state) {
        case IDLE:
            if (byte & SIXTH_BIT) {
                buttons = (byte >> 4) & BOTTOM_TWO_BITS;
                yd = ((byte & THIRD_FOURTH_BITS) << 4);
                xd = ((byte & BOTTOM_TWO_BITS) << 6);
                state = XADD;
            }
            break;

        case XADD:
            xd |= (byte & BOTTOM_SIX_BITS);
            state = YADD;
            break;

        case YADD:
            yd |= (byte & BOTTOM_SIX_BITS);
            state = IDLE;
            if (xd > 127)
                xd -= 256;
            if (yd > 127)
                yd -= 256;
            return 1;
    }
    return 0;
}

/*
 * Input routine for PS/2 mouse.
 * Returns nonzero when a new mouse state has been completed.
 */
static int parsePS2(int byte)
{
    switch (state) {
    case IDLE:
        if (byte & PS2_CTRL_BYTE) {
            buttons = byte & (PS2_LEFT_BUTTON|PS2_RIGHT_BUTTON);
            state = XSET;
        }
        break;

    case XSET:
        if(byte > 127)
                byte -= 256;
        xd = byte;
        state = YSET;
        break;

    case YSET:
        if(byte > 127)
                byte -= 256;
        yd = -byte;
        state = IDLE;
        return 1;
    }
    return 0;
}

#ifndef DRIVER
int main(int argc, char **argv)
{
    int x, y, z, b;

    rawmode = (argc > 1);

    if(open_mouse() < 0)
        return 1;

    printf("[Mouse test, ^C to quit]\n");
    while(1) {
        if(read_mouse(&x, &y, &z, &b) == 1) {
            printf("x:%4d, y:%4d, b:%2d\n", x, y, b);
        }
    }
    return 0;
}
#endif
