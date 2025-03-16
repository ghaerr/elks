/*
 * Cursor routines from Microwindows
 *
 * Device-independent top level mouse and cursor routines
 * Reads data from mouse driver and tracks real position on the screen.
 * Intersection detection for cursor with auto removal

 * Copyright (c) 1999, 2000, 2002, 2010, 2011 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Copyright (C) 1999 Bradley D. LaRonde (brad@ltc.com)
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */
#include <string.h>
#include "graphics.h"
#include "event.h"
#include "mouse.h"

#define USE_SMALL_CURSOR    0       /* =1 small cursor for slow systems*/

#define MWMAX_CURSOR_SIZE   16      /* maximum cursor x and y size*/
#define MWMAX_CURSOR_BUFLEN MWIMAGE_SIZE(MWMAX_CURSOR_SIZE,MWMAX_CURSOR_SIZE)

/* MWIMAGEBITS macros*/
#define MWIMAGE_WORDS(x)    (((x)+15)/16)
#define MWIMAGE_BYTES(x)    (MWIMAGE_WORDS(x)*sizeof(MWIMAGEBITS))
/* size of image in words*/
#define MWIMAGE_SIZE(width, height)     \
    ((height) * (((width) + MWIMAGE_BITSPERIMAGE - 1) / MWIMAGE_BITSPERIMAGE))
#define MWIMAGE_BITSPERIMAGE    (sizeof(MWIMAGEBITS) * 8)
#define MWIMAGE_BITVALUE(n) ((MWIMAGEBITS) (((MWIMAGEBITS) 1) << (n)))
#define MWIMAGE_FIRSTBIT    (MWIMAGE_BITVALUE(MWIMAGE_BITSPERIMAGE - 1))
#define MWIMAGE_NEXTBIT(m)  ((MWIMAGEBITS) ((m) >> 1))
#define MWIMAGE_TESTBIT(m)  ((m) & MWIMAGE_FIRSTBIT)  /* use with shiftbit*/
#define MWIMAGE_SHIFTBIT(m) ((MWIMAGEBITS) ((m) << 1))  /* for testbit*/

#define TRUE            1
#define FALSE           0

static int  curminx;        /* minimum x value of cursor */
static int  curminy;        /* minimum y value of cursor */
static int  curmaxx;        /* maximum x value of cursor */
static int  curmaxy;        /* maximum y value of cursor */
static int  curvisible;     /* >0 if cursor is visible*/
static int  curneedsrestore;/* cursor needs restoration after drawing*/
static int  cursavx;        /* saved cursor location*/
static int  cursavy;
static int  cursavx2;
static int  cursavy2;
typedef int MWPIXELVALHW;
static MWPIXELVALHW curfg;      /* foreground hardware pixel value */
static MWPIXELVALHW curbg;      /* background hardware pixel value */
static MWPIXELVALHW cursavbits[MWMAX_CURSOR_SIZE * MWMAX_CURSOR_SIZE];
static MWIMAGEBITS cursormask[MWMAX_CURSOR_BUFLEN];
static MWIMAGEBITS cursorcolor[MWMAX_CURSOR_BUFLEN];

void initcursor(void)
{
#if USE_SMALL_CURSOR
/* Small 8x8 cursor for machines to slow for 16x16 cursors */
    static MWIMAGEBITS cursormask[8];
    static MWIMAGEBITS cursorbits[8];
#define HOTX    1
#define HOTY    1
#define CURSW   8
#define CURSH   8
    cursormask[0] = MASK(X,X,X,X,X,X,_);
    cursormask[1] = MASK(X,X,X,X,X,_,_);
    cursormask[2] = MASK(X,X,X,X,_,_,_);
    cursormask[3] = MASK(X,X,X,X,X,_,_);
    cursormask[4] = MASK(X,X,X,X,X,X,_);
    cursormask[5] = MASK(X,_,_,X,X,X,X);
    cursormask[6] = MASK(_,_,_,_,X,X,X);

    cursorbits[0] = MASK(_,_,_,_,_,_,_);
    cursorbits[1] = MASK(_,X,X,X,X,_,_);
    cursorbits[2] = MASK(_,X,X,X,_,_,_);
    cursorbits[3] = MASK(_,X,X,X,_,_,_);
    cursorbits[4] = MASK(_,X,_,X,X,_,_);
    cursorbits[5] = MASK(_,_,_,_,X,X,_);
    cursorbits[6] = MASK(_,_,_,_,_,X,X);

#else

#define HOTX    0
#define HOTY    0
#define CURSW   16
#define CURSH   16
    static MWIMAGEBITS cursorbits[16] = {
          0xe000, 0x9800, 0x8600, 0x4180,
          0x4060, 0x2018, 0x2004, 0x107c,
          0x1020, 0x0910, 0x0988, 0x0544,
          0x0522, 0x0211, 0x000a, 0x0004
    };
    static MWIMAGEBITS cursormask[16] = {
          0xe000, 0xf800, 0xfe00, 0x7f80,
          0x7fe0, 0x3ff8, 0x3ffc, 0x1ffc,
          0x1fe0, 0x0ff0, 0x0ff8, 0x077c,
          0x073e, 0x021f, 0x000e, 0x0004
    };
#endif
    static struct cursor cursor = {
        CURSW, CURSH, HOTX, HOTY, WHITE, BLACK, cursorbits, cursormask
    };

    /* init cursor position and size*/
    curvisible = 0;
    curneedsrestore = FALSE;
    curminx = 0;
    curminy = 0;
    setcursor(&cursor);
    movecursor(posx, posy);
    showcursor();
}

/**
 * Set the cursor position.
 *
 * @param newx New X co-ordinate.
 * @param newy New Y co-ordinate.
 */
void movecursor(int newx, int newy)
{
    int shiftx;
    int shifty;

    shiftx = newx - curminx;
    shifty = newy - curminy;
    if(shiftx == 0 && shifty == 0)
        return;
    curminx += shiftx;
    curmaxx += shiftx;
    curminy += shifty;
    curmaxy += shifty;

    /* Restore the screen under the mouse pointer*/
    hidecursor();

    /* Draw the new pointer*/
    showcursor();
}

/**
 * Set the cursor size and bitmaps.
 *
 * @param pcursor New mouse cursor.
 */
void setcursor(struct cursor *pcursor)
{
    int bytes;

    hidecursor();
    curmaxx = curminx + pcursor->width - 1;
    curmaxy = curminy + pcursor->height - 1;

    //curfg = GdFindColor(pcursor->fgcolor);
    //curbg = GdFindColor(pcursor->bgcolor);
    curfg = pcursor->fgcolor;
    curbg = pcursor->bgcolor;

    bytes = MWIMAGE_SIZE(pcursor->width, pcursor->height) * sizeof(MWIMAGEBITS);
    memcpy(cursorcolor, pcursor->image, bytes);
    memcpy(cursormask, pcursor->mask, bytes);

    showcursor();
}


/**
 * Draw the mouse pointer.  Save the screen contents underneath
 * before drawing. Returns previous cursor state.
 *
 * @return 1 iff the cursor was visible, else <= 0
 */
int showcursor(void)
{
    int             x;
    int             y;
    MWPIXELVALHW *  saveptr;
    MWIMAGEBITS *   cursorptr;
    MWIMAGEBITS *   maskptr;
    MWIMAGEBITS     curbit, cbits = 0, mbits = 0;
    MWPIXELVALHW    oldcolor;
    MWPIXELVALHW    newcolor;
    int             prevcursor = curvisible;

    if(++curvisible != 1)
        return prevcursor;

    saveptr = cursavbits;
    cursavx = curminx;
    cursavy = curminy;
    cursavx2 = curmaxx;
    cursavy2 = curmaxy;
    cursorptr = cursorcolor;
    maskptr = cursormask;

    /*
     * Loop through bits, resetting to firstbit at end of each row
     */
    curbit = 0;
    for (y = curminy; y <= curmaxy; y++) {
        if (curbit != MWIMAGE_FIRSTBIT) {
            cbits = *cursorptr++;
            mbits = *maskptr++;
            curbit = MWIMAGE_FIRSTBIT;
        }
        for (x = curminx; x <= curmaxx; x++) {
            if(x >= 0 && x < SCREENWIDTH && y >= 0 && y < SCREENHEIGHT) {
                oldcolor = readpixel(x, y);
                if (curbit & mbits) {
                    newcolor = (curbit&cbits)? curbg: curfg;
                    if (oldcolor != newcolor)
                        drawpixel(x, y, newcolor);
                }
                *saveptr++ = oldcolor;
            }
            curbit = MWIMAGE_NEXTBIT(curbit);
            if (!curbit) {  /* check > one MWIMAGEBITS wide*/
                cbits = *cursorptr++;
                mbits = *maskptr++;
                curbit = MWIMAGE_FIRSTBIT;
            }
        }
    }

    return prevcursor;
}

/**
 * Restore the screen overwritten by the cursor.
 *
 * @return 1 iff the cursor was visible, else <= 0
 */
int hidecursor(void)
{
    MWPIXELVALHW *  saveptr;
    int             x, y;
    int             prevcursor = curvisible;

    if(curvisible-- <= 0)
        return prevcursor;

    saveptr = cursavbits;
    for (y = cursavy; y <= cursavy2; y++) {
        for (x = cursavx; x <= cursavx2; x++) {
            if(x >= 0 && x < SCREENWIDTH && y >= 0 && y < SCREENHEIGHT) {
                drawpixel(x, y, *saveptr++);
            }
        }
    }
    return prevcursor;
}

#if UNUSED
/**
 * Check to see if the mouse pointer is about to be overwritten.
 * If so, then remove the cursor so that the graphics operation
 * works correctly.  If the cursor is removed, then this fact will
 * be remembered and a later call to fixcursor will restore it.
 *
 * @param x1 Left edge of rectangle to check.
 * @param y1 Top edge of rectangle to check.
 * @param x2 Right edge of rectangle to check.
 * @param y2 Bottom edge of rectangle to check.
 */
void checkcursor(int x1,int y1,int x2,int y2)
{
    int temp;

    if (curvisible <= 0)
        return;

    if (x1 > x2) {
        temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y2) {
        temp = y1;
        y1 = y2;
        y2 = temp;
    }
    if (x1 > curmaxx || x2 < curminx || y1 > curmaxy || y2 < curminy)
        return;

    hidecursor();
    curneedsrestore = TRUE;
}

/**
 * Remove cursor from display, restored with fixcursor.
 */
void erasecursor(void)
{
    if (curvisible <= 0)
        return;

    hidecursor();
    curneedsrestore = TRUE;
}

/**
 * Redisplay the cursor if it was removed because of a graphics operation.
 */
void fixcursor(void)
{
    if (curneedsrestore) {
        showcursor();
        curneedsrestore = FALSE;
    }
}
#endif
