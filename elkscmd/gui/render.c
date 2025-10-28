#include <stdlib.h>
#include <string.h>   /* for memcpy */
#include "app.h"
#include "render.h"
#include "graphics.h"
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
    R_DrawDisk(x2, y2, bushSize, color, CANVAS_WIDTH);
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
    drawvline(xmin, ymin+1, ymax-1, color);
    drawvline(xmax, ymin+1, ymax-1, color);
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
// R_LineFloodFill: Line Flood Fill, for the bucket tool
// Original slow but simple flood fill
// Used by 'f' key and C86, C86 can't return structs used in new routine
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

#ifndef __C86__     /* very fast fill routine written by Vutshi, uses struct returns  */

/* max segments in any one SegList */
#define MAX_SEGS        18
/* max SegLists we expect to push per stack */
#define MAX_LISTS       4
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
// static void fill_segs(const SegList *E, int new_color) {
//     for (int i = 0; i < E->n; i++) {
//         drawhline(E->s[i].xl, E->s[i].xr, E->y, new_color);
//     }
// }
#define FILL_SEGS(E, color)                                         \
    do {                                                            \
        int __i;                                                    \
        for (__i = 0; __i < (E)->n; __i++)                           \
            drawhline((E)->s[__i].xl,                                \
                      (E)->s[__i].xr,                                \
                      (E)->y,                                        \
                      (color));                                     \
    } while (0)


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

#define EXPAND(result, x, y, target)                      \
    do {                                                         \
        int _xl = (x), _xr = (x);                                \
        while (_xl > 0 && readpixel(_xl - 1, (y)) == target) \
            _xl--;                                           \
        while (_xr < CANVAS_WIDTH-1 && readpixel(_xr + 1, (y)) == target) \
            _xr++;                                           \
        (result).xl = _xl; (result).xr = _xr;                \
    } while (0)


/* lookup table for number of leading 1s in a 4‑bit nibble
   index = nibble value (0x0–0xF), value = count of 1‑bits
   starting at the nibble’s MSB */
static const unsigned short L1[16] = {
    /*0x0*/ 0, /*0x1*/ 0, /*0x2*/ 0, /*0x3*/ 0,
    /*0x4*/ 0, /*0x5*/ 0, /*0x6*/ 0, /*0x7*/ 0,
    /*0x8*/ 1, /*0x9*/ 1, /*0xA*/ 1, /*0xB*/ 1,
    /*0xC*/ 2, /*0xD*/ 2, /*0xE*/ 3, /*0xF*/ 4
};

/*
 * Count the run of 1‑bits from bit 7 downward.
 * – Extract the high nibble, look up its leading‑1 count.
 * – If it was all ones (0xF), continue into the low nibble.
 */
static int count_leading_ones(unsigned short x) {
    unsigned short hi = x >> 4;
    int cnt = L1[hi];
    if (hi == 0x0F) {
        /* high nibble was 1111, so add low‑nibble’s leading‑1 count */
        cnt += L1[x & 0x0F];
    }
    return cnt-1;
}

/* lookup table for number of trailing 1s in a 4‑bit nibble */
static const unsigned short T1[16] = {
    /* 0x0 */ 0,     /* 0x1 */ 1,    /* 0x2 */ 0,     /* 0x3 */ 2,
    /* 0x4 */ 0,     /* 0x5 */ 1,    /* 0x6 */ 0,     /* 0x7 */ 3,
    /* 0x8 */ 0,     /* 0x9 */ 1,    /* 0xA */ 0,     /* 0xB */ 2,
    /* 0xC */ 0,     /* 0xD */ 1,    /* 0xE */ 0,     /* 0xF */ 4
};

/*
 * Count the run of 1‐bits from bit 0 upward.
 * For low nibble ≠ 0xF this is just one table lookup.
 * If low nibble == 0xF, we get 4 + table[high_nibble].
 */
static int count_trailing_ones(unsigned short x) {
    unsigned short lo = x & 0x0F;
    int cnt = T1[lo];
    if (lo == 0x0F) {
        /* still all 1s in low nibble, so add count from high nibble */
        cnt += T1[x >> 4];
    }
    return cnt-1;
}


/*
    * expand_cmp8:
    *   full two‑sided expansion around seed (x,y) for 'target' color.
    */
static inline Segment expand_cmp8(int x, unsigned int offset_y, int target) {
    Segment seg = { 0, -1 };

    /* seed must match */
    unsigned short m = asm_getbyte(offset_y + (x >> 3));
    int bit = x & 7;

    seg.xl = seg.xr = x;

    /* —— grow left within this byte —— */
    int b = bit;
    while (b > 0 && (m & (1 << (7 - (b - 1))))) {
        b--; seg.xl--;
    }
    /* —— only if we reached the leftmost bit of this byte, go byte‑wise —— */
    if (b == 0) {
        int bx = (x >> 3) - 1;
        while (bx >= 0) {
            unsigned short m2 = asm_getbyte(offset_y + bx);
            if (m2 == 0xFF) {
                seg.xl = (bx<<3);
                bx--;
            } else {
                int hb = count_trailing_ones(m2);
                if (hb >= 0)
                    seg.xl = (bx<<3) + (7 - hb);
                break;
            }
        }
    }

    /* —— grow right within this byte —— */
    b = bit;
    while (b < 7 && (m & (1 << (7 - (b + 1))))) {
        b++; seg.xr++;
    }
    /* —— only if we reached the rightmost bit of this byte, go byte‑wise —— */
    if (b == 7) {
        int bx = (x >> 3) + 1;
        int maxBX = (CANVAS_WIDTH>>3);
        while (bx <= maxBX) {
            unsigned short m2 = asm_getbyte(offset_y + bx);
            if (m2 == 0xFF) {
                seg.xr = (bx<<3) + 7;
                bx++;
            } else {
                int lb = count_leading_ones(m2);
                if (lb >= 0)
                    seg.xr = (bx<<3) + lb;
                break;
            }
        }
    }

    if (seg.xr >= CANVAS_WIDTH)
            seg.xr = CANVAS_WIDTH - 1;
    return seg;
}

/* initialize VGA to Read Mode 1 & set compare/color mask */
/* FIXME ia16 & c86 only for now: set_color_compare takes non-constant parameter */
static void vga_cmp8_init(int target_color)
{
    /* Set Graphics Mode Register to read mode 1 (bit 3 = 1) */
    set_write_mode(8);

    /* Set Color Compare Register to target color (4 bits) */
    set_color_compare(target_color & 0x0F);

    /* Set Color Don't Care Register to 0x0F (include all planes) */
    set_color_dont_care(0x0F);
}

/* LINK: scan one row above/below E for 4-connected runs under E.s */
static SegList LINK(const SegList *E, int target) {
    SegList out;
    out.y   = E->y + E->dir;
    out.dir = E->dir;
    out.n   = 0;
    if (out.y < 0 || out.y >= CANVAS_HEIGHT || E->n == 0)
        return out;

    unsigned int offset_y = (out.y<<6) + (out.y<<4);

    /* scan across the union of parent spans */
    int i =  0;
    int x =  E->s[0].xl;
    /* Initialize Color Compare Mode to target color */
    vga_cmp8_init(target);
    while (1) {
        /* skip past any parent that ends before x */
        while (i < E->n && x > E->s[i].xr) i++;
        if (i >= E->n) break;
        /* snap to start of this span if x too small */
        if (x < E->s[i].xl) x = E->s[i].xl;

        /* if we hit the target, expand and record */
        if (asm_getbyte(offset_y + (x >> 3)) & (1 << (7 - (x & 7)))) {
        // if (readpixel(x, out.y) == target) {
            Segment seg_new;
            seg_new.xl = x;
            seg_new.xr = x;
            seg_new = expand_cmp8(x, offset_y, target);
            // EXPAND(seg_new, x, out.y, target);

            if (seg_new.xl <= seg_new.xr && out.n < MAX_SEGS) {
                out.s[out.n++] = seg_new;
                x = seg_new.xr + 2;  /* jump past */
                continue;
            }
        }
        x++;
    }
    /* Restore default write mode */
    set_write_mode(0);
    return out;
}

/* DIFF: Ep \ (each Ed.s[j] expanded ±1) → new runs going opposite dir */
/* Both Ep and Ed have segments sorted with no overlaps. */
static SegList DIFF(const SegList *Ep, const SegList *Ed) {
    SegList out;
    out.y   = Ep->y;
    out.dir = -Ep->dir;
    out.n   = 0;

    int j = 0;  /* pointer into Ed->s[] */

    for (int i = 0; i < Ep->n; i++) {
        int cur_l = Ep->s[i].xl;
        int cur_r = Ep->s[i].xr;

        /* advance j past any forbidden intervals ending before cur_l */
        while (j < Ed->n && Ed->s[j].xr + 1 < cur_l) {
            j++;
        }
        /* now subtract all forbidden intervals that overlap [cur_l..cur_r] */
        int k = j;
        while (k < Ed->n) {
            int fl = Ed->s[k].xl - 1;  /* forbidden left */
            if (fl > cur_r)            /* no more overlaps */
                break;

            int fr = Ed->s[k].xr + 1;  /* forbidden right */

            /* output any gap before this forbidden block */
            if (fl > cur_l) {
                out.s[out.n].xl = cur_l;
                out.s[out.n++].xr = fl - 1;
            }
            /* advance cur_l past the forbidden block */
            if (fr + 1 > cur_l) {
                cur_l = fr + 1;
            }
            if (cur_l > cur_r)
                break;  /* fully consumed */
            k++;
        }
        /* if any tail remains, output it */
        if (cur_l <= cur_r) {
            out.s[out.n].xl = cur_l;
            out.s[out.n++].xr = cur_r;
        }
    }
    return out;
}

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
 * Precondition: Ep->y == top.y
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

     n1--;
     n2--;
     while (n1 >= 0 && n2 >= 0) {
         if (p1[n1].xl > Ep->s[n2].xl) {
             *dest = p1[n1--];
         } else {
             *dest = Ep->s[n2--];
         }
         dest--;
     }
     // copy any remaining from seg1
     while (n1 >= 0) {
         *dest = p1[n1--];
         dest--;
     }
     // copy any remaining from Ep
     while (n2 >= 0) {
         *dest = Ep->s[n2--];
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
    Segment s0;
    s0.xl = x0;
    s0.xr = x0;
    // EXPAND(s0, x0, y0, targetColor);
    vga_cmp8_init(targetColor);
    s0 = expand_cmp8(x0, (y0<<6) + (y0<<4), targetColor);
    set_write_mode(0);

    E.y = y0;
    E.dir = +1;
    E.n = 1;
    E.s[0] = s0;

    /* fill initial */
    FILL_SEGS(&E, newColor);

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
        FILL_SEGS(&Ep, newColor);

        /* backward leak */
        SegList Em = DIFF(&Ep, &E);

        /* merge or push Ep */
        /* peek top metadata to decide */
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
#endif /* !__C86__ */

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
