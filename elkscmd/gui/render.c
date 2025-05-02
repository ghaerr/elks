#include <stdlib.h>
#include "app.h"
#include "render.h"
#include "graphics.h"
#include <string.h>   /* for memcpy */
#include "vgalib.h"


#define FLOOD_FILL_STACK    200

// ----------------------------------------------------
// Clear the screen
// ----------------------------------------------------
void R_ClearCanvas(void)
{
    fillrect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT-1, BLACK);
}


// ----------------------------------------------------
// Draws the Palette
// ----------------------------------------------------
void R_DrawPalette()
{
    // Draw line and background
    drawvline(CANVAS_WIDTH+1, 0, CANVAS_HEIGHT-1, WHITE);
    fillrect(CANVAS_WIDTH + 2, 0,
        CANVAS_WIDTH + PALETTE_WIDTH - 1, CANVAS_HEIGHT - 1, GRAY);

    R_UpdateColorPicker();
    R_DrawAllButtons();

    // Draw Logo if VGA 640x480
    if(CANVAS_HEIGHT == 480)
        draw_bmp(LIBPATH "paint.bmp", CANVAS_WIDTH + 10, 360);
}

// ----------------------------------------------------
// Draws all the Palette Buttons
// ----------------------------------------------------
void R_DrawAllButtons()
{
    for(int i = 1; i < PALETTE_BUTTONS_COUNT; i++) {
        if(paletteButtons[i].render == true) {
            if(paletteButtons[i].data1 > 0) {
                DrawButtonCircle(paletteButtons[i].box.x, paletteButtons[i].box.y,
                    paletteButtons[i].box.w, paletteButtons[i].box.h, paletteButtons[i].data1);
            } else {
                draw_bmp(paletteButtons[i].fileName,
                    paletteButtons[i].box.x, paletteButtons[i].box.y);
            }
        }
    }
}

// ----------------------------------------------------
// Draws Brush Buttons
// ----------------------------------------------------
void DrawButtonCircle(int x0, int y0, int w, int h, int r)
{
    fillrect(x0, y0, x0 + w-1, y0 + h-1, WHITE);
    R_DrawDisk(x0 + (w>>1), y0 + (h>>1), r, BLACK, SCREENWIDTH);
}

#if UNUSED
static int MapRGB(int r, int g, int b)
{
    r = (r & 0x80) >> 7;
    g = (g & 0xC0) >> 6;
    b = (b & 0x80) >> 7;
    return ((r << 3) | (g << 1) | b);
}
#endif

// ----------------------------------------------------
// Updates the Color Picker when brightness changes.
// ----------------------------------------------------
void R_UpdateColorPicker(void)
{
    // Draw RGB scheme
    int startingPixelXOffset = CANVAS_WIDTH + 10 + 1;
    int startingPixelYOffset = 10 + 1;

    //static ColorRGB_t color;
    //ColorHSV_t hsv;
    //hsv.h = x*2;
    //hsv.s = y * 4;
    //hsv.v = paletteBrightness;
    //color = HSVtoRGB(hsv);
    for(int cy = 0; cy < 4; cy++) {
        for(int cx = 0; cx < 4; cx++) {
            int color = cx + (cy<<2);
            int x0 = (cx<<5)+startingPixelXOffset;
            int y0 = (cy<<5)+startingPixelYOffset;
            fillrect(x0, y0, x0 + 30-1, y0 + 30-1, color);
        }
    }

    R_DrawCurrentColor();
}

// ----------------------------------------------------
// Updates the Current Color when changes.
// ----------------------------------------------------
void R_DrawCurrentColor(void)
{
    // Draw current color
    int startingPixelXOffset = CANVAS_WIDTH + 10 + 32 + 1;
    int startingPixelYOffset = 10 + 128 + 10;

    fillrect(startingPixelXOffset, startingPixelYOffset, startingPixelXOffset + 61,
        startingPixelYOffset + 29, currentMainColor);

    // Draw white frame if the main color matches the panel backgound
    if (currentMainColor == GRAY) {
        drawhline(startingPixelXOffset, startingPixelXOffset + 61, 0 + startingPixelYOffset, WHITE);
        drawhline(startingPixelXOffset, startingPixelXOffset + 61, 29+ startingPixelYOffset, WHITE);
        drawvline(0 +startingPixelXOffset, startingPixelYOffset, 29 + startingPixelYOffset, WHITE);
        drawvline(61+startingPixelXOffset, startingPixelYOffset, 29 + startingPixelYOffset, WHITE);
    }
}
// ----------------------------------------------------
// Highlight the button corresponding to the active drawing mode.
// ----------------------------------------------------
void R_HighlightActiveButton(void)
{
    int x1 = paletteButtons[currentModeButton].box.x + 1;
    int x2 = x1 + paletteButtons[currentModeButton].box.w - 3;
    int y1 = paletteButtons[currentModeButton].box.y + 1;
    int y2 = y1 + paletteButtons[currentModeButton].box.h - 3;
    set_op(0x18);    // turn on XOR drawing
    fillrect(x1, y1, x2, y2, WHITE);
    set_op(0);       // turn off XOR drawing
}

// ----------------------------------------------------
// Paint
// ----------------------------------------------------
void R_Paint(int x1, int y1, int x2, int y2) {
    int color = current_color;
    // Draw initial point
    R_DrawDisk(x1, y1, bushSize, color, CANVAS_WIDTH);

    // Bresenham's line algorithm for efficient line drawing
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (x1 != x2 || y1 != y2) {
        R_DrawDisk(x1, y1, bushSize, color, CANVAS_WIDTH);

        int e2 = err << 1;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

// ----------------------------------------------------
// Used to draw at 2+px bush size.
// ----------------------------------------------------
void R_DrawDisk(int x0, int y0, int r, int color, int X_lim)
// Based on Algorithm http://members.chello.at/easyfilter/bresenham.html
{
    if (r <= 1) {
        drawpixel(x0, y0, color);
        return;
    }
    int x = -r, y = 0, err = 2-2*r; /* II. Quadrant */
    do {
        int xstart = (x0 + x >= 0) ? x0 + x : 0;
        int xend   = (x0 - x <= X_lim) ? x0 - x : X_lim;

        if (y0 + y < CANVAS_HEIGHT)
            drawhline(xstart, xend, y0 + y, color);

        if (y0 - y >= 0 && y > 0)
            drawhline(xstart, xend, y0 - y, color);

        r = err;
        if (r <= y) err +=  ++y*2+1;          /* e_xy+e_y < 0 */
        if (r > x || err > y) err += ++x*2+1; /* e_xy+e_x > 0 or no 2nd y-step */
    }
    while (x < 0);
}

void R_DrawCircle(int x0, int y0, int r, int color)
{
    int x = -r, y = 0, err = 2-2*r; /* II. Quadrant */

    while (-x >= y) {
        // Draw symmetry points
        int x1 = (x0 + x >= 0) ? x0 + x : 0;
        int x2   = (x0 - x <= CANVAS_WIDTH) ? x0 - x : CANVAS_WIDTH;
        int y1 = (y0 - y >= 0) ? y0 - y : 0;
        int y2   = (y0 + y < CANVAS_HEIGHT) ? y0 + y : CANVAS_HEIGHT-1;
        drawpixel(x1, y2, color);
        drawpixel(x2, y2, color);
        drawpixel(x1, y1, color);
        drawpixel(x2, y1, color);
        x1 = (x0 - y >= 0) ? x0 - y : 0;
        x2   = (x0 + y <= CANVAS_WIDTH) ? x0 + y : CANVAS_WIDTH;
        y1 = (y0 + x >= 0) ? y0 + x : 0;
        y2   = (y0 - x < CANVAS_HEIGHT) ? y0 - x : CANVAS_HEIGHT-1;
        drawpixel(x2, y1, color);
        drawpixel(x1, y1, color);
        drawpixel(x2, y2, color);
        drawpixel(x1, y2, color);

        r = err;
        if (r <= y) err +=  ++y*2+1;          /* e_xy+e_y < 0 */
        if (r > x || err > y) err += ++x*2+1; /* e_xy+e_x > 0 or no 2nd y-step */
    }
}

void R_DrawRectangle(int x1, int y1, int x2, int y2)
{
    int color = current_color;
    int xmin, xmax, ymin, ymax;

    // Normalize coordinates
    xmin = (x1 <= x2) ? x1 : x2;
    xmax = (x1 > x2) ? x1 : x2;
    ymin = (y1 <= y2) ? y1 : y2;
    ymax = (y1 > y2) ? y1 : y2;

    // Top and bottom horizontal lines
    drawhline(xmin, xmax, ymin, color);
    drawhline(xmin, xmax, ymax, color);

    // Left and right vertical lines
    drawvline(xmin, ymin, ymax, color);
    drawvline(xmax, ymin, ymax, color);
}

void R_DrawFilledRectangle(int x1, int y1, int x2, int y2)
{
    int color = current_color;
    int xmin, xmax, ymin, ymax;

    // Normalize coordinates
    xmin = (x1 <= x2) ? x1 : x2;
    xmax = (x1 > x2) ? x1 : x2;
    ymin = (y1 <= y2) ? y1 : y2;
    ymax = (y1 > y2) ? y1 : y2;

    fillrect(xmin, ymin, xmax, ymax, color);
}

// ----------------------------------------------------
// Flood Fill Stack Operations
// ----------------------------------------------------
static void FF_StackPush(transform2d_t stack[], int x, int y, int* top)
{
    if (*top < FLOOD_FILL_STACK-1) {
        (*top)++;
        stack[*top].x = x;
        stack[*top].y = y;
    }
}

static transform2d_t FF_StackPop(transform2d_t stack[], int* top)
{
    return stack[(*top)--];
}

// ----------------------------------------------------
// Line Flood Fill, for the bucket tool
// ----------------------------------------------------
void R_LineFloodFill(int x, int y, int color, int ogColor)
{
    int stackTop = -1;
    boolean_t mRight;
    boolean_t alreadyCheckedAbove, alreadyCheckedBelow;
    transform2d_t curElement;
    transform2d_t stack[FLOOD_FILL_STACK];

    if(color == ogColor)
        return;

    // Push the first element
    FF_StackPush(stack, x, y, &stackTop);
    while(stackTop >= 0)    // While there are elements
    {
        // Take the first one
#ifdef __C86__     /* FIXME C86 compiler bug FF_StackPop returning struct*/
        curElement = stack[stackTop];
        stackTop--;
#else
        curElement = FF_StackPop(stack, &stackTop);
#endif
        mRight = false;
        int leftestX = curElement.x;

        // Find leftest
        while(leftestX >= 0 && readpixel(leftestX, curElement.y) == ogColor)
            leftestX--;
        leftestX++;

        alreadyCheckedAbove = false;
        alreadyCheckedBelow = false;

        // While this line has not finsihed to be drawn
        while(mRight == false)
        {
            // Fill right
            if(leftestX < CANVAS_WIDTH && readpixel(leftestX, curElement.y) == ogColor)
            {
                drawpixel(leftestX, curElement.y, color);

                // Check above this pixel
                if (alreadyCheckedBelow == false && (curElement.y-1) >= 0
                    && (curElement.y-1) < CANVAS_HEIGHT
                    && readpixel(leftestX, curElement.y-1) == ogColor)
                    {
                        // If we never checked it, add it to the stack
                        FF_StackPush(stack, leftestX, curElement.y-1, &stackTop);
                        alreadyCheckedBelow = true;
                    }
                else if(alreadyCheckedBelow == true && (curElement.y-1) > 0
                        && readpixel(leftestX, curElement.y-1) != ogColor)
                {
                    // Skip now, but check next time
                    alreadyCheckedBelow = false;
                }

                // Check below this pixel
                if (alreadyCheckedAbove == false && (curElement.y+1) >= 0
                    && (curElement.y+1) < CANVAS_HEIGHT
                    && readpixel(leftestX, curElement.y+1) == ogColor)
                    {
                        // If we never checked it, add it to the stack
                        FF_StackPush(stack, leftestX, curElement.y+1, &stackTop);
                        alreadyCheckedAbove = true;
                    }
                else if (alreadyCheckedAbove == true
                    && (curElement.y+1) < CANVAS_HEIGHT
                    && readpixel(leftestX, curElement.y+1) != ogColor)
                {
                    // Skip now, but check next time
                    alreadyCheckedAbove = false;
                }

                // Keep going on the right
                leftestX++;
            }
            else // Done
                mRight = true;
        }
    }
}


#define STACK_SIZE 200

// #if UNUSED
/* A shadow record for span-based stack fill */
typedef struct {
    int myLx, myRx;       /* endpoints of this shadow span */
    int dadLx, dadRx;     /* endpoints of parent span */
    int myY;              /* scanline of this span */
    int myDirection;      /* +1 = below parent, -1 = above parent */
} StackElement;

/* Stack and pointer */
static StackElement stack[STACK_SIZE];
static int top = 0;

/* Push a new shadow span */
#define PUSH(l, r, dl, dr, yy, dir) \
    do { if (top < STACK_SIZE - 1) \
        stack[top++] = (StackElement){ (l), (r), (dl), (dr), (yy), (dir) }; } while (0)

/* Pop the top shadow into locals */
#define POP() \
    do { \
        StackElement _e = stack[--top]; \
        lx       = _e.myLx; \
        rx       = _e.myRx; \
        dadLx    = _e.dadLx; \
        dadRx    = _e.dadRx; \
        y        = _e.myY; \
        direction= _e.myDirection; \
    } while (0)

/* Handle new span discovery (S/U/W turns) */
#define STACK(dir, dL, dR, l, r, yy)             \
    do {                                        \
        int _pushrx = (r) + 1;                  \
        int _pushlx = (l) - 1;                  \
        PUSH((l),   (r),   _pushlx, _pushrx, (yy) + (dir), (dir)); \
        if ((r) > (dR))                         \
            PUSH((dR) + 1, (r), _pushlx, dR, (yy) - (dir), -(dir)); \
        if ((l) < (dL))                         \
            PUSH((l),   (dL) - 1, dL, _pushrx, (yy) - (dir), -(dir)); \
    } while (0)

/*
 * Fill:
 *   Starting at (seedX, seedY), flood-fill all contiguous pixels
 *   whose readpixel(x,y) == targetColor, coloring each via drawpixel(x,y,newColor).
 *
 * Parameters:
 *   seedX, seedY  : seed point
 *   targetColor   : pixel value to replace
 *   newColor      : new pixel value to draw
 */
void R_SpanFloodFill(int seedX, int seedY, int newColor, int targetColor)
{
    int lx, rx, dadLx, dadRx, y, direction;
    int x;
    int wasIn;

    if(newColor == targetColor)
        return;

    /* Find initial span around seed */
    lx = seedX;  rx = seedX;
    while (lx > 0 && readpixel(lx - 1, seedY) == targetColor)
        lx--;
    while (rx < CANVAS_WIDTH - 1 && readpixel(rx + 1, seedY) == targetColor)
        rx++;

    drawhline(lx, rx, seedY, newColor);

    /* Initialize stack: below and above */
    PUSH(lx, rx, lx, rx, seedY + 1,  1);
    PUSH(lx, rx, lx, rx, seedY - 1, -1);

    /* Process spans */
    while (top > 0) {
        POP();  /* sets lx,rx,dadLx,dadRx,y,direction */

        /* Skip outside vertical limits (0 to CANVAS_HEIGHT) */
        if (y < 0 || y > CANVAS_HEIGHT)
            continue;
        // if (rx == CANVAS_WIDTH - 1) drawhline(lx, rx, y - direction, newColor);

        /* Prepare for scanning */
        x = lx + 1;
        wasIn = (readpixel(lx, y) == targetColor);

        /* Extend left if starting inside */
        if (wasIn) {
            drawpixel(lx, y, newColor);
            lx--;
            while (lx >= 0 && readpixel(lx, y) == targetColor) {
                drawpixel(lx, y, newColor);
                lx--;
            }
        }
        lx++;
        // drawhline(lx, x - 1, y, newColor);

        /* Scan across line */
        while (x <= CANVAS_WIDTH - 1) {
            if (wasIn) {
                if (readpixel(x, y) == targetColor) {
                    /* case 1: still in span */
                    drawpixel(x, y, newColor);
                } else {
                    /* case 2: exit span */
                    // drawhline(lx, x - 1, y, newColor);
                    STACK(direction, dadLx, dadRx, lx, x - 1, y);
                    wasIn = 0;
                }
            } else {
                if (x > rx)
                    break;
                if (readpixel(x, y) == targetColor) {
                    /* case 3: enter new span */
                    drawpixel(x, y, newColor);
                    wasIn = 1;
                    lx = x;
                }
            }
            x++;
        }
        /* If still in span at edge, push it */
        if (wasIn)
            // drawhline(lx, x - 1, y, newColor);
            STACK(direction, dadLx, dadRx, lx, x - 1, y);
    }
}
// #endif



#if UNUSED
/* shadow “span” record, packed into 6×16 bits = 12 bytes each */
typedef struct {
    short myLx,   /* left  x of this span */
          myRx,   /* right x of this span */
          dadLx,  /* left  x of parent span */
          dadRx,  /* right x of parent span */
          myY,    /* y scanline of this span */
          myDir;  /* +1 = below parent, -1 = above */
} StackElement;

/* one global stack and pointer */
static StackElement stack[STACK_SIZE];
static int           top = 0;

void FastSpanFloodFill(int seedX, int seedY, int newColor, int targetColor)
{
    /* locals */
    register int sp = top;                   /* local stack pointer */
    register StackElement *stk = stack;     /* base of stack */
    register int lx, rx, dadLx, dadRx, y, dir;
    register int x, inSpan;

    /* trivial reject */
    if (newColor == targetColor)
        return;

    /* 1) find initial contiguous span around seed */
    lx = seedX;  rx = seedX;
    while (lx > 0 && readpixel(lx-1, seedY) == targetColor) lx--;
    while (rx < CANVAS_WIDTH-1 && readpixel(rx+1, seedY) == targetColor) rx++;
    drawhline(lx, rx, seedY, newColor);

    /* 2) seed two spans: below (+1) and above (−1) */
    if (sp < STACK_SIZE-1) {
        stk[sp].myLx     = lx;
        stk[sp].myRx     = rx;
        stk[sp].dadLx    = lx;
        stk[sp].dadRx    = rx;
        stk[sp].myY      = seedY + 1;
        stk[sp].myDir    =  1;
        sp++;
    }
    if (sp < STACK_SIZE-1) {
        stk[sp].myLx     = lx;
        stk[sp].myRx     = rx;
        stk[sp].dadLx    = lx;
        stk[sp].dadRx    = rx;
        stk[sp].myY      = seedY - 1;
        stk[sp].myDir    = -1;
        sp++;
    }

    /* 3) process until stack empty */
    while (sp > 0) {
        /* pop top */
        {
            StackElement e = stk[--sp];
            lx    = e.myLx;   rx    = e.myRx;
            dadLx = e.dadLx;  dadRx = e.dadRx;
            y     = e.myY;    dir   = e.myDir;
        }

        /* skip if scanline is out of bounds */
        if ((unsigned)y >= CANVAS_HEIGHT)
            continue;

        /* start just right of lx */
        x      = lx + 1;
        inSpan = (readpixel(lx, y) == targetColor);

        /* if we start “in” the target, extend left */
        if (inSpan) {
            drawpixel(lx--, y, newColor);
            while (lx >= 0 && readpixel(lx, y) == targetColor) {
                drawpixel(lx--, y, newColor);
            }
            lx++;
        }

        /* now scan across [lx..rx] */
        while (x <= rx) {
            if (inSpan) {
                if (readpixel(x, y) == targetColor) {
                    drawpixel(x, y, newColor);
                } else {
                    /* 1) push the filled span on the “same-direction” line */
                    {
                        int l = lx, r = x - 1;
                        int pl = l - 1, pr = r + 1;
                        if (sp < STACK_SIZE-1) {
                            stk[sp].myLx  = l;
                            stk[sp].myRx  = r;
                            stk[sp].dadLx = pl;
                            stk[sp].dadRx = pr;
                            stk[sp].myY   = y + dir;
                            stk[sp].myDir = dir;
                            sp++;
                        }
                        /* 2) “leak” to opposite side if r > dadRx */
                        if (r > dadRx && sp < STACK_SIZE-1) {
                            stk[sp].myLx  = dadRx + 1;
                            stk[sp].myRx  = r;
                            stk[sp].dadLx = pl;
                            stk[sp].dadRx = dadRx;
                            stk[sp].myY   = y - dir;
                            stk[sp].myDir = -dir;
                            sp++;
                        }
                        /* 3) “leak” on left if lx < dadLx */
                        if (l < dadLx && sp < STACK_SIZE-1) {
                            stk[sp].myLx  = l;
                            stk[sp].myRx  = dadLx - 1;
                            stk[sp].dadLx = dadLx;
                            stk[sp].dadRx = pr;
                            stk[sp].myY   = y - dir;
                            stk[sp].myDir = -dir;
                            sp++;
                        }
                    }
                    inSpan = 0;
                }
            } else if (readpixel(x, y) == targetColor) {
                /* enter a new span */
                drawpixel(x, y, newColor);
                inSpan = 1;
                lx     = x;
            }
            x++;
        }

        /* if we ended still “in” target, push that last span */
        if (inSpan) {
            int l = lx, r = x - 1;
            int pl = l - 1, pr = r + 1;
            if (sp < STACK_SIZE-1) {
                stk[sp].myLx  = l;
                stk[sp].myRx  = r;
                stk[sp].dadLx = pl;
                stk[sp].dadRx = pr;
                stk[sp].myY   = y + dir;
                stk[sp].myDir = dir;
                sp++;
            }
            if (r > dadRx && sp < STACK_SIZE-1) {
                stk[sp].myLx  = dadRx + 1;
                stk[sp].myRx  = r;
                stk[sp].dadLx = pl;
                stk[sp].dadRx = dadRx;
                stk[sp].myY   = y - dir;
                stk[sp].myDir = -dir;
                sp++;
            }
            if (l < dadLx && sp < STACK_SIZE-1) {
                stk[sp].myLx  = l;
                stk[sp].myRx  = dadLx - 1;
                stk[sp].dadLx = dadLx;
                stk[sp].dadRx = pr;
                stk[sp].myY   = y - dir;
                stk[sp].myDir = -dir;
                sp++;
            }
        }
    }

    /* write back stack pointer */
    top = sp;
}
#endif

#if UNUSED
//****
//****
//****
//****
typedef struct {int y, xl, xr, dy;} Segment;
/*
 * Filled horizontal segment of scanline y for xl<=x<=xr.
 * Parent segment was on line y-dy.  dy=1 or -1
 */

#define PUSH_SEG(Y, XL, XR, DY)	/* push new segment on stack */ \
    if (sp<stack+STACK_SIZE && Y+(DY)>=0 && Y+(DY) < CANVAS_HEIGHT) \
    {sp->y = Y; sp->xl = XL; sp->xr = XR; sp->dy = DY; sp++;}

#define POP_SEG(Y, XL, XR, DY)	/* pop segment off stack */ \
    {sp--; Y = sp->y+(DY = sp->dy); XL = sp->xl; XR = sp->xr;}

/*
 * fill: set the pixel at (x,y) and all of its 4-connected neighbors
 * with the same pixel value to the new pixel value nv.
 * A 4-connected neighbor is a pixel above, below, left, or right of a pixel.
 */

void R_SpanFloodFill_v2(int x, int y, int newColor, int targetColor)
{
    int l, x1, x2, dy;
    Segment stack[STACK_SIZE], *sp = stack;	/* stack of filled segments */

    if(newColor == targetColor)
        return;
    PUSH_SEG(y, x, x, 1);			/* needed in some cases */
    PUSH_SEG(y+1, x, x, -1);		/* seed segment (popped 1st) */

    while (sp>stack) {
    	/* pop segment off stack and fill a neighboring scan line */
    	POP_SEG(y, x1, x2, dy);
    	/*
    	 * segment of scan line y-dy for x1<=x<=x2 was previously filled,
    	 * now explore adjacent pixels in scan line y
    	 */
    	for (x=x1; x>=0 && readpixel(x, y)==targetColor; x--)
    	    drawpixel(x, y, newColor);
    	if (x>=x1) goto skip;
    	l = x+1;
    	if (l<x1) PUSH_SEG(y, l, x1-1, -dy);		/* leak on left? */
    	x = x1+1;
    	do {
    	    for (; x<=CANVAS_WIDTH - 1 && readpixel(x, y)==targetColor; x++)
      		drawpixel(x, y, newColor);
    	    PUSH_SEG(y, l, x-1, dy);
    	    if (x>x2+1) PUSH_SEG(y, x2+1, x-1, -dy);	/* leak on right? */
skip:	    for (x++; x<=x2 && readpixel(x, y)!=targetColor; x++);
    	    l = x;
    	} while (x<=x2);
    }
}




void FastSpanFloodFill_v2(int sx, int sy, int newColor, int targetColor)
{
    /* nothing to do if colors match */
    if (newColor == targetColor)
        return;

    /* stack in static storage for faster 8088 access */
    static Segment stack[STACK_SIZE];
    int sp = 0;

    /* seed segments: one going up, one going down */
    stack[sp++] = (Segment){ sy,  sx,  sx, -1 };
    stack[sp++] = (Segment){ sy,  sx,  sx,  1 };

    while (sp > 0) {
        /* pop a segment */
        Segment s = stack[--sp];
        register int cy = s.y + s.dy;
        register int xl = s.xl;
        register int xr = s.xr;
        register int x;

        /* scan left from xl */
        x = xl;
        while (x >= 0 && readpixel(x, cy) == targetColor) {
            drawpixel(x--, cy, newColor);
        }
        int left = x + 1;

        /* scan right from xl+1 */
        x = xl + 1;
        while (x < CANVAS_WIDTH && readpixel(x, cy) == targetColor) {
            drawpixel(x++, cy, newColor);
        }
        int right = x - 1;

        /* push the just-filled span so we can handle the next line in same direction */
        stack[sp++] = (Segment){ cy, left, right, s.dy };

        /* if there’s unfilled “leak” to the left of the original xl, push it (opposite direction) */
        if (left < xl) {
            stack[sp++] = (Segment){ cy, left, xl - 1, -s.dy };
        }
        /* likewise for any leak to the right of the original xr */
        if (right > xr) {
            stack[sp++] = (Segment){ cy, xr + 1, right, -s.dy };
        }
    }
}
#endif



/* max segments in any one SegList */
#define MAX_SEGS        18
/* max SegLists we expect to push per stack */
#define MAX_LISTS       18
/* each push uses 2 ints per segment + 3 ints of metadata */
#define STACK_STRIDE    (MAX_SEGS*2 + 3)
/* total ints in stack buffer */
#define MAX_STACK_SIZE  (STACK_STRIDE * MAX_LISTS)

/* one horizontal run [xl..xr] */
typedef struct { int xl, xr; } Segment;

/* temporary working SegList */
typedef struct {
    int       y, dir, n;
    Segment   s[MAX_SEGS];
} SegList;

/* fill every pixel in E.s on row E.y */
static void fill_segs(const SegList *E, int new_color) {
    for (int i = 0; i < E->n; i++) {
        drawhline(E->s[i].xl, E->s[i].xr, E->y, new_color);
    }
}

// /* EXPAND: full two-sided run around (x,y) if it matches target, else empty */
// static Segment expand(int x, int y, int target) {
//     Segment seg = {0, -1};
//     if (readpixel(x, y) != target)
//         return seg;
//     seg.xl = seg.xr = x;
//     while (seg.xl > 0 && readpixel(seg.xl - 1, y) == target) seg.xl--;
//     while (seg.xr < CANVAS_WIDTH-1 && readpixel(seg.xr + 1, y) == target) seg.xr++;
//     return seg;
// }
// /* EXPAND_RIGHT: only to the right from x (already target) */
// static Segment expand_right(int x, int y, int target) {
//     Segment seg = {x, x};
//     while (seg.xr < CANVAS_WIDTH-1 && readpixel(seg.xr + 1, y) == target)
//         seg.xr++;
//     return seg;
// }

#define EXPAND(result, x, y, target, width)                      \
    do {                                                         \
        int _xl = (x), _xr = (x);                                \
        while (_xl > 0 && readpixel(_xl - 1, (y)) == target) \
            _xl--;                                           \
        while (_xr < (width)-1 && readpixel(_xr + 1, (y)) == target) \
            _xr++;                                           \
        (result).xl = _xl; (result).xr = _xr;                \
    } while (0)

#define EXPAND_RIGHT(result, x, y, target, width)                 \
    do {                                                          \
        int _xr = (x);                                            \
        while (_xr < (width)-1 && readpixel(_xr + 1, (y)) == target) \
            _xr++;                                                \
        (result).xl = (x);                                        \
        (result).xr = _xr;                                        \
    } while (0)


/* LINK: scan one row above/below E for 4-connected runs under E.s */
static SegList LINK(const SegList *E, int target) {
    SegList out;
    out.y   = E->y + E->dir;
    out.dir = E->dir;
    out.n   = 0;
    if (out.y < 0 || out.y >= CANVAS_HEIGHT || E->n == 0)
        return out;

    /* scan across the union of parent spans */
    int i =  0;
    int x =  E->s[0].xl;
    while (1) {
        /* skip past any parent that ends before x */
        while (i < E->n && x > E->s[i].xr) i++;
        if (i >= E->n) break;
        /* snap to start of this span if x too small */
        if (x < E->s[i].xl) x = E->s[i].xl;

        /* if we hit the target, expand and record */
        if (readpixel(x, out.y) == target) {
            Segment seg_new = {x, x};
            if (x == E->s[i].xl)
                EXPAND(seg_new, x, out.y, target, CANVAS_WIDTH);
            else
                EXPAND_RIGHT(seg_new, x, out.y, target, CANVAS_WIDTH);
            // Segment seg_new = (x == E->s[i].xl)
            //                   ? expand(x, out.y, target)
            //                   : expand_right(x, out.y, target);
            if (seg_new.xl <= seg_new.xr && out.n < MAX_SEGS) {
                out.s[out.n++] = seg_new;
                x = seg_new.xr + 2;  /* jump past */
                continue;
            }
        }
        x++;
    }
    return out;
}

/* DIFF: Ep \ (each Ed.s[j] expanded ±1) → new runs going opposite dir */
static SegList DIFF(const SegList *Ep, const SegList *Ed) {
    SegList out;
    out.y   = Ep->y;
    out.dir = -Ep->dir;
    out.n   = 0;
    for (int i = 0; i < Ep->n; i++) {
        int cur_l = Ep->s[i].xl;
        int cur_r = Ep->s[i].xr;
        for (int j = 0; j < Ed->n; j++) {
            int fl = Ed->s[j].xl - 1;
            int fr = Ed->s[j].xr + 1;
            if (fr < cur_l)      continue;
            if (fl > cur_r)      break;
            if (fl > cur_l)
                out.s[out.n++] = (Segment){ cur_l, fl - 1 };
            if (fr + 1 > cur_l)
                cur_l = fr + 1;
            if (cur_l > cur_r)
                break;
        }
        if (cur_l <= cur_r)
            out.s[out.n++] = (Segment){ cur_l, cur_r };
    }
    return out;
}

// /* MERGE two sorted, non-overlapping lists into one (no touching) */
// static SegList MERGE(const SegList *A, const SegList *B) {
//     SegList out;  out.y = A->y;  out.dir = A->dir;  out.n = 0;
//     int i = 0, j = 0;
//     while (i < A->n && j < B->n) {
//         if (A->s[i].xl <= B->s[j].xl)
//             out.s[out.n++] = A->s[i++];
//         else
//             out.s[out.n++] = B->s[j++];
//     }
//     while (i < A->n) out.s[out.n++] = A->s[i++];
//     while (j < B->n) out.s[out.n++] = B->s[j++];
//     return out;
// }



/* One flat stack type for SegLists */
typedef struct {
    int    data[MAX_STACK_SIZE];
    int   *sp;      /* next free slot */
    int    cap;     /* sum of all n’s currently in stack */
} SegStack;

/* Initialize a stack */
static void stack_init(SegStack *S) {
    S->sp  = S->data;
    S->cap = 0;
}

/* Push E onto the stack with one memcpy */
static void stack_push(SegStack *S, const SegList *E) {
    int needed = E->n*2 + 3;                // # of ints we need
    int used   = (int)(S->sp - S->data);    // current usage
    if (E->n <= 0 || used + needed > MAX_STACK_SIZE)
        return;

    /* copy segments (2*E->n ints) */
    memcpy(S->sp,            /* dest */
           E->s,             /* src: pointer is okay because Segment is two ints */
           E->n * sizeof(Segment));
    /* advance pointer by # of ints */
    S->sp += E->n * 2;

    /* now metadata: y, dir, n */
    S->sp[0] = E->y;
    S->sp[1] = E->dir;
    S->sp[2] = E->n;
    S->sp   += 3;

    S->cap  += E->n;
}

/* Pop into *E using one memcpy for the segments */
static int stack_pop(SegStack *S, SegList *E) {
    /* need at least 3 ints for metadata */
    if ((S->sp - S->data) < 3) return 0;

    /* metadata lives in the last three ints */
    int *meta = S->sp - 3;
    int n     = meta[2];
    int block = n*2 + 3;
    /* ensure stack has enough ints */
    if ((S->sp - S->data) < block) return 0;

    /* locate segment data start */
    int *seg_start = S->sp - block;

    /* restore metadata */
    E->y   = meta[0];
    E->dir = meta[1];
    E->n   = n;

    /* bulk-copy segments back into E->s */
    memcpy(E->s,            /* dest */
           seg_start,       /* src: xl0,xr0,xl1,xr1,… */
           n * sizeof(Segment));

    /* rewind stack pointer */
    S->sp  -= block;
    S->cap -= n;
    return 1;
}

/*
 * Merge Ep directly into the top‐of‐stack SegList inside S, in place.
 * Precondition: Ep->y == top.y  &&  Ep->dir == top.dir
 *               top has n1 segments at seg1_start,
 *               Ep has n2 segments in Ep->s[0..n2-1].
 * Postcondition: the top now has (n1+n2) segments, merged in-order,
 *               metadata updated, sp and cap adjusted.
 */
 static void merge_Ep_inplace(SegStack *S, const SegList *Ep) {
     // ----- 1) Locate top‐of‐stack metadata and its segments -----
     int *meta1      = S->sp - 3;
     int y_top       = meta1[0];
     int dir_top     = meta1[1];
     int n1          = meta1[2];
     int *seg1_start = meta1 - (n1 * 2);

     int n2 = Ep->n;
     if (n2 <= 0) return;            // nothing to merge
     if ((int)(S->sp - S->data) + (n2 * 2) > MAX_STACK_SIZE)
         return;                     // overflow guard

     // ----- 2) Reserve room for the extra segments -----
     int total = n1 + n2;
     S->sp   += (n2 * 2);            // grow segment‐block by n2*2 ints
     S->cap  +=  n2;                 // grow capacity by n2
     // ----- Rewrite merged metadata [y, dir, total] -----
     int *meta_out = S->sp - 3;
     meta_out[0]   = y_top;          // y
     meta_out[1]   = dir_top;        // dir
     meta_out[2]   = total;          // new n

     // ----- 3) Merge backwards: seg1_start[0..2*n1-1] and Ep->s[] -----
     Segment *p1   = (Segment *)seg1_start;   // cast to Segment pointer
     Segment *dest = (Segment *)(seg1_start + (total-1)*2); // cast to Segment pointer

     int i1 = n1-1;
     int i2 = n2-1;
     while (i1 >= 0 && i2 >= 0) {
         if (p1[i1].xl > Ep->s[i2].xl) {
             *dest = p1[i1--];
         } else {
             *dest = Ep->s[i2--];
         }
         dest--;
     }
     // copy any remaining from seg1
     while (i1 >= 0) {
         *dest = p1[i1--];
         dest--;
     }
     // copy any remaining from Ep
     while (i2 >= 0) {
         *dest = Ep->s[i2--];
         dest--;
     }
 }

/*
 * FRONT_FILL: iterative 4-connected flood-fill
 * starting at (x0,y0), painting targetColor → newColor.
 * Returns peak capacity seen (optional).
 */
int R_FrontFill(int x0, int y0, int newColor, int targetColor) {
    if (newColor == targetColor) return 0;

    SegStack S_plus, S_minus;
    stack_init(&S_plus);
    stack_init(&S_minus);
    SegList E;

    /* seed span on row y0 */
    Segment s0 = {x0, x0};
    EXPAND(s0, x0, y0, targetColor, CANVAS_WIDTH);

    E.y = y0;
    E.dir = +1;
    E.n = 1;
    E.s[0] = s0;

    /* fill initial */
    fill_segs(&E, newColor);

    /* push seeds */
    stack_push(&S_plus,  &E);
    E.dir = -1;
    stack_push(&S_minus, &E);

    int maxCap = S_plus.cap > S_minus.cap ? S_plus.cap : S_minus.cap;

    /* main loop: always pop from the stack with larger capacity */
    while (S_plus.cap > 0 || S_minus.cap > 0) {
        SegStack *Sa = (S_plus.cap >= S_minus.cap) ? &S_plus : &S_minus;
        SegStack *Sp = (Sa == &S_plus ? &S_minus : &S_plus);

        /* pop one segment list */
        if (!stack_pop(Sa, &E))
            break;

        /* forward link + fill */
        SegList Ep = LINK(&E, targetColor);
        fill_segs(&Ep, newColor);

        /* backward leak */
        SegList Em = DIFF(&Ep, &E);

        /* merge or push Ep */
        /* peek top metadata if any */
        int *meta = Sa->sp - 3;
        int y_top = meta[0];
        if ((Sa->cap == 0) || (y_top != Ep.y))
            stack_push(Sa, &Ep);
        else
            merge_Ep_inplace(Sa, &Ep);

        /* push the leak */
        stack_push(Sp, &Em);

        /* update peak capacity */
        if (Sa->cap > maxCap)
            maxCap = Sa->cap;
    }
    return maxCap;
}





#if UNUSED
// ----------------------------------------------------
// Converts from HSV to RGB
// ----------------------------------------------------
ColorRGB_t HSVtoRGB(ColorHSV_t colorHSV)
{
    float r, g, b, h, s, v; //this function works with floats between 0 and 1
    h = colorHSV.h / 256.0;
    s = colorHSV.s / 256.0;
    v = colorHSV.v / 256.0;

    //If saturation is 0, the color is a shade of gray
    if(s == 0) r = g = b = v;
    //If saturation > 0, more complex calculations are needed
    else
    {
        float f, p, q, t;
        int i;
        h *= 6; //to bring hue to a number between 0 and 6, better for the calculations
        i = (int)(floor(h));  //e.g. 2.7 becomes 2 and 3.01 becomes 3 or 4.9999 becomes 4
        f = h - i;  //the fractional part of h
        p = v * (1 - s);
        q = v * (1 - (s * f));
        t = v * (1 - (s * (1 - f)));
        switch(i)
        {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }
    }


    ColorRGB_t colorRGB;
    colorRGB.r = (int)(r * 255.0);
    colorRGB.g = (int)(g * 255.0);
    colorRGB.b = (int)(b * 255.0);
    return colorRGB;
}
#endif
