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
#include "vgalib.h"

#define USE_XOR_CURSOR      1       /* =1 use XOR drawpixel vs full cursor & mask draw */
#define USE_VGA_DRAWCURSOR  (__ia16__ || __WATCOMC__)   /* VGA hardware XOR cursor */

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
static MWIMAGEBITS cursormask[MWMAX_CURSOR_BUFLEN];
static MWIMAGEBITS cursorcolor[MWMAX_CURSOR_BUFLEN];
#if !USE_VGA_DRAWCURSOR
static MWPIXELVALHW cursavbits[MWMAX_CURSOR_SIZE * MWMAX_CURSOR_SIZE];
#endif

/* Small 8x8 cursor for machines to slow for 16x16 cursors */
#if USE_XOR_CURSOR
    static MWIMAGEBITS smcursorbits[8] = {
        mask(X,_,_,_,_,_,_,_),
        mask(X,X,_,_,_,_,_,_),
        mask(X,_,X,_,_,_,_,_),
        mask(X,_,_,X,_,_,_,_),
        mask(X,_,_,_,X,_,_,_),
        mask(X,_,X,X,X,X,_,_),
        mask(X,X,_,_,_,_,_,_),
        mask(X,_,_,_,_,_,_,_)
    };
    static MWIMAGEBITS smcursormask[8] = {
        mask(X,_,_,_,_,_,_,_),
        mask(X,X,_,_,_,_,_,_),
        mask(X,X,X,_,_,_,_,_),
        mask(X,X,X,X,_,_,_,_),
        mask(X,X,X,X,X,_,_,_),
        mask(X,X,X,X,X,X,_,_),
        mask(X,X,_,_,_,_,_,_),
        mask(X,_,_,_,_,_,_,_)
    };
    struct cursor cursor_sm = {
        6, 8, 1, 1, WHITE, BLACK, smcursorbits, smcursormask
    };
#else
    static MWIMAGEBITS smcursormask[8] = {
        mask(X,_,_,_,_,_,_,_),
        mask(X,X,_,_,_,_,_,_),
        mask(X,X,X,_,_,_,_,_),
        mask(X,X,X,X,_,_,_,_),
        mask(X,X,X,X,X,_,_,_),
        mask(X,X,X,X,X,X,_,_),
        mask(X,X,X,X,X,X,X,_),
        mask(X,X,X,_,_,_,_,_)
    };
    static MWIMAGEBITS smcursorbits[8] = {
        mask(_,_,_,_,_,_,_,_),
        mask(_,_,_,_,_,_,_,_),
        mask(_,X,_,_,_,_,_,_),
        mask(_,X,X,_,_,_,_,_),
        mask(_,X,X,X,_,_,_,_),
        mask(_,X,X,X,X,_,_,_),
        mask(_,X,X,_,_,_,_,_),
        mask(_,_,_,_,_,_,_,_)
    };
    struct cursor cursor_sm = {
        7, 8, 1, 1, WHITE, BLACK, smcursorbits, smcursormask
    };
#endif

/* Full size 16x16 cursor */
#ifdef __WATCOMC__
static MWIMAGEBITS lgcursorbits[16] = {
//       8 4 2 1 8 4 2 1 8 4 2 1 8 4 2 1
    MASK(X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_),  // E000
    MASK(X,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_),  // 9800
    MASK(X,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_),  // 8600
    MASK(_,X,_,_,_,_,_,X,X,_,_,_,_,_,_,_),  // 4180
    MASK(_,X,_,_,_,_,_,_,_,X,X,_,_,_,_,_),  // 4060
    MASK(_,_,X,_,_,_,_,_,_,_,_,X,X,_,_,_),  // 2018
    MASK(_,_,X,_,_,_,_,_,_,_,_,_,_,X,_,_),  // 2004
    MASK(_,_,_,X,_,_,_,_,_,X,X,X,X,X,_,_),  // 107C
    MASK(_,_,_,X,_,_,_,_,_,_,X,_,_,_,_,_),  // 1020
    MASK(_,_,_,_,X,_,_,X,_,_,_,X,_,_,_,_),  // 0910
    MASK(_,_,_,_,X,_,_,X,_,_,_,_,X,_,_,_),  // 0988
    MASK(_,_,_,_,_,X,_,X,_,X,_,_,_,X,_,_),  // 0544
    MASK(_,_,_,_,_,X,_,X,_,_,X,_,_,_,X,_),  // 0522
    MASK(_,_,_,_,_,_,X,_,_,_,_,X,_,_,_,X),  // 0211
    MASK(_,_,_,_,_,_,_,_,_,_,_,_,X,_,X,_),  // 000A
    MASK(_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_)   // 0004
};
static MWIMAGEBITS lgcursormask[16] = {
//       8 4 2 1 8 4 2 1 8 4 2 1 8 4 2 1
    MASK(X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_),  // E000
    MASK(X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_),  // F800
    MASK(X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_),  // FE00
    MASK(_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_),  // 7F80
    MASK(_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_),  // 7FE0
    MASK(_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_),  // 3FF8
    MASK(_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_),  // 3FFC
    MASK(_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_),  // 1FFC
    MASK(_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_),  // 1FE0
    MASK(_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_),  // 0FF0
    MASK(_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_),  // 0FF8
    MASK(_,_,_,_,_,X,X,X,_,X,X,X,X,X,_,_),  // 077C
    MASK(_,_,_,_,_,X,X,X,_,_,X,X,X,X,X,_),  // 073E
    MASK(_,_,_,_,_,_,X,_,_,_,_,X,X,X,X,X),  // 021F
    MASK(_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_),  // 000E
    MASK(_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_)   // 0004
};

struct cursor cursor_lg = {
    16, 16, 0, 0, WHITE, BLACK, lgcursorbits, lgcursormask
};
#else
static MWIMAGEBITS lgcursorbits[16] = {
//       8 4 2 1 8 4 2 1 8 4 2 1 8 4 2 1
    MASK(X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
    MASK(X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
    MASK(X,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_),
    MASK(X,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_),
    MASK(X,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_),
    MASK(X,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_),
    MASK(X,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_),
    MASK(X,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_),
    MASK(X,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_),
    MASK(X,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_),
    MASK(X,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_),
    MASK(X,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_),
    MASK(X,_,_,X,_,_,X,_,_,_,_,_,_,_,_,_),
    MASK(X,_,X,_,X,_,_,X,_,_,_,_,_,_,_,_),
    MASK(X,X,_,_,X,_,_,X,_,_,_,_,_,_,_,_),
    MASK(X,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_)
};
static MWIMAGEBITS lgcursormask[16] = {
//       8 4 2 1 8 4 2 1 8 4 2 1 8 4 2 1
    MASK(X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
    MASK(X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
    MASK(X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_),
    MASK(X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_),
    MASK(X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_),
    MASK(X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_),
    MASK(X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_),
    MASK(X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_),
    MASK(X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_),
    MASK(X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_),
    MASK(X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_),
    MASK(X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_),
    MASK(X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_),
    MASK(X,X,X,_,X,X,X,X,_,_,_,_,_,_,_,_),
    MASK(X,X,_,_,X,X,X,X,_,_,_,_,_,_,_,_),
    MASK(X,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_)
};

struct cursor cursor_lg = {
    11, 16, 0, 0, WHITE, BLACK, lgcursorbits, lgcursormask
};
#endif

void initcursor(void)
{
    /* init cursor position and size*/
    curvisible = 0;
    curneedsrestore = FALSE;
    curminx = 0;
    curminy = 0;
    // set small cursor for EGA
    setcursor((SCREENHEIGHT < 480) ? &cursor_sm : &cursor_lg);
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
#if !USE_VGA_DRAWCURSOR
    MWIMAGEBITS *   maskptr = cursormask;
    MWIMAGEBITS     curbit = 0;
    MWIMAGEBITS     mbits = 0;
    int             x, y;
#endif
    int             prevcursor = curvisible;

    if(++curvisible != 1)
        return prevcursor;

    cursavx = curminx;
    cursavy = curminy;
    cursavx2 = curmaxx;
    cursavy2 = curmaxy;

    /*
     * Loop through bits, resetting to firstbit at end of each row
     */
#if USE_VGA_DRAWCURSOR
    vga_drawcursor(curminx, curminy, curmaxy - curminy + 1, cursormask);
#elif USE_XOR_CURSOR
    set_op(0x18);
    for (y = curminy; y <= curmaxy; y++) {
        if (curbit != MWIMAGE_FIRSTBIT) {
            mbits = *maskptr++;
            curbit = MWIMAGE_FIRSTBIT;
        }
        for (x = curminx; x <= curmaxx; x++) {
            if (curbit & mbits) {
                if(x >= 0 && x < SCREENWIDTH && y >= 0 && y < SCREENHEIGHT) {
                    drawpixel(x, y, ~0);
                }
            }
            curbit = MWIMAGE_NEXTBIT(curbit);
            if (!curbit) {  /* check > one MWIMAGEBITS wide*/
                mbits = *maskptr++;
                curbit = MWIMAGE_FIRSTBIT;
            }
        }
    }
    set_op(0);
#else
    MWPIXELVALHW *  saveptr = cursavbits;
    MWIMAGEBITS *   cursorptr = cursorcolor;
    MWIMAGEBITS     cbits = 0;
    MWPIXELVALHW    oldcolor;
    MWPIXELVALHW    newcolor;

    for (y = curminy; y <= curmaxy; y++) {
        if (curbit != MWIMAGE_FIRSTBIT) {
            cbits = *cursorptr++;
            mbits = *maskptr++;
            curbit = MWIMAGE_FIRSTBIT;
        }
        for (x = curminx; x <= curmaxx; x++) {
            if (curbit & mbits) {
                if (x >= 0 && x < SCREENWIDTH && y >= 0 && y < SCREENHEIGHT) {
                    oldcolor = readpixel(x, y);
                    newcolor = (curbit&cbits)? curbg: curfg;
                    if (oldcolor != newcolor)
                        drawpixel(x, y, newcolor);
                    *saveptr++ = oldcolor;
                }
            }
            curbit = MWIMAGE_NEXTBIT(curbit);
            if (!curbit) {  /* check > one MWIMAGEBITS wide*/
                cbits = *cursorptr++;
                mbits = *maskptr++;
                curbit = MWIMAGE_FIRSTBIT;
            }
        }
    }
#endif

    return prevcursor;
}

/**
 * Restore the screen overwritten by the cursor.
 *
 * @return 1 iff the cursor was visible, else <= 0
 */
int hidecursor(void)
{
#if !USE_VGA_DRAWCURSOR
    MWIMAGEBITS *   maskptr = cursormask;
    MWIMAGEBITS     curbit = 0;
    MWIMAGEBITS     mbits = 0;
    int             x, y;
#endif
    int             prevcursor = curvisible;

    if(curvisible-- <= 0)
        return prevcursor;

#if USE_VGA_DRAWCURSOR
    vga_drawcursor(cursavx, cursavy, cursavy2 - cursavy + 1, cursormask);
#elif USE_XOR_CURSOR
    set_op(0x18);
    for (y = cursavy; y <= cursavy2; y++) {
        if (curbit != MWIMAGE_FIRSTBIT) {
            mbits = *maskptr++;
            curbit = MWIMAGE_FIRSTBIT;
        }
        for (x = cursavx; x <= cursavx2; x++) {
            if (curbit & mbits) {
                if (x >= 0 && x < SCREENWIDTH && y >= 0 && y < SCREENHEIGHT) {
                    drawpixel(x, y, ~0);
                }
            }
            curbit = MWIMAGE_NEXTBIT(curbit);
            if (!curbit) {  /* check > one MWIMAGEBITS wide*/
                mbits = *maskptr++;
                curbit = MWIMAGE_FIRSTBIT;
            }
        }
    }
    set_op(0);
#else
    MWPIXELVALHW *  saveptr = cursavbits;

    for (y = cursavy; y <= cursavy2; y++) {
        if (curbit != MWIMAGE_FIRSTBIT) {
            mbits = *maskptr++;
            curbit = MWIMAGE_FIRSTBIT;
        }
        for (x = cursavx; x <= cursavx2; x++) {
            if (curbit & mbits) {
                if (x >= 0 && x < SCREENWIDTH && y >= 0 && y < SCREENHEIGHT) {
                    drawpixel(x, y, *saveptr++);
                }
            }
            curbit = MWIMAGE_NEXTBIT(curbit);
            if (!curbit) {  /* check > one MWIMAGEBITS wide*/
                mbits = *maskptr++;
                curbit = MWIMAGE_FIRSTBIT;
            }
        }
    }
#endif
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
