#include <stdlib.h>
#include "app.h"
#include "render.h"
#include "graphics.h"

#define FLOOD_FILL_STACK    100

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
        draw_bmp(LIBPATH "paint.bmp", CANVAS_WIDTH + 10, 350);
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
// Paint
// ----------------------------------------------------
void R_Paint(int x1, int y1, int x2, int y2) {
    int color = drawing ? currentMainColor : currentAltColor;
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

// ----------------------------------------------------
// Flood Fill Stack Operations
// ----------------------------------------------------
static void FF_StackPush(transform2d_t stack[], int x, int y, int* top)
{
    if (*top < FLOOD_FILL_STACK) {
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
                //pixels[leftestX + curElement.y * width] = color;
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
                    && (curElement.y+1) < CANVAS_WIDTH
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
