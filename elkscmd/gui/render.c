#include <stdlib.h>
#include "app.h"
#include "render.h"
#include "event.h"
#include "graphics.h"

#define FLOOD_FILL_STACK    100

// ----------------------------------------------------
// Given an X pixel, draw the whole column
// ----------------------------------------------------
void R_DrawFullColumn(int x, int color)
{
    for(int y = 0; y < SCREEN_HEIGHT; y++)
        drawpixel(x, y, color);
}

// ----------------------------------------------------
// Clear the screen
// ----------------------------------------------------
void R_ClearCanvas(void)
{
#ifdef __ia16__
    for(int y = 0; y < SCREEN_HEIGHT; y++)
        drawhline(0, SCREEN_WIDTH-1, y, BLACK);
#else
    for(int y = 0; y < SCREEN_HEIGHT; y++)
      for(int x = 0; x < SCREEN_WIDTH; x++)
        drawpixel(x, y, BLACK);
#endif
}


// ----------------------------------------------------
// Draws the Palette
// ----------------------------------------------------
void R_DrawPalette()
{
    // Draw line and background
    R_DrawFullColumn(SCREEN_WIDTH+1, WHITE);
    int paletteX = SCREEN_WIDTH+2;

#ifdef __ia16__
    for(int y=0; y<SCREEN_HEIGHT; y++)
        drawhline(paletteX, SCREEN_WIDTH + PALETTE_WIDTH - 1, y, GRAY);
#else
    while(paletteX < SCREEN_WIDTH + PALETTE_WIDTH)
        R_DrawFullColumn(paletteX++, GRAY);
#endif
    R_UpdateColorPicker();

    R_DrawAllButtons();

    // Draw Logo if VGA 640x480
    if(SCREEN_HEIGHT == 480)
        draw_bmp(LIBPATH "paint.bmp", SCREEN_WIDTH + 10, 350);
}

// ----------------------------------------------------
// Draws all the Palette Buttons
// ----------------------------------------------------
void R_DrawAllButtons()
{
    for(int i = 1; i < PALETTE_BUTTONS_COUNT; i++)
    {
        if(paletteButtons[i].render == true)
        {
            if(paletteButtons[i].data1 > 0)
            {
                DrawButtonCircle(paletteButtons[i].box.x, paletteButtons[i].box.y,
                    paletteButtons[i].box.w, paletteButtons[i].box.h, paletteButtons[i].data1);
            }
            else
            {
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
    // Double values for half-pixel precision and
    // smoother circles by centering them on the middle of a pixel
    int cx0 = 2*x0 + w - 1;
    int cy0 = 2*y0 + h - 1;
    for(int y=y0; y<y0 + h; y++)
        for(int x=x0; x<x0 + w; x++)
        {
            int color = ((2*x-cx0)*(2*x-cx0)+(2*y-cy0)*(2*y-cy0) <= 4*r*r) ? BLACK : WHITE;
            drawpixel(x, y, color);
        }
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
    int startingPixelXOffset = SCREEN_WIDTH + 10 + 1;
    int startingPixelYOffset = 10 + 1;

    for(int x = 0; x < 128; x++)
      for(int y = 0; y < 128; y++)
      {
        //static ColorRGB_t color;
        //ColorHSV_t hsv;
        //hsv.h = x*2;
        //hsv.s = y * 4;
        //hsv.v = paletteBrightness;
        //color = HSVtoRGB(hsv);
        int color = (x>>5) + (y>>5) * 4;
        //draw gray bars
        color = ((x - (x>>5)*32) >=30) | ((y - (y>>5)*32)>=30) ? GRAY : color;
        drawpixel(x+startingPixelXOffset, y + startingPixelYOffset, color);
      }

    R_DrawCurrentColor();
}

// ----------------------------------------------------
// Updates the Current Color when changes.
// ----------------------------------------------------
void R_DrawCurrentColor(void)
{
    // Draw current color
    int startingPixelXOffset = SCREEN_WIDTH + 10 + 32 + 1;
    int startingPixelYOffset = 10 + 128 + 10;

    for(int x = 0; x < 62; x++)
        for(int y = 0; y < 30; y++)
            drawpixel(x+startingPixelXOffset, y + startingPixelYOffset, currentMainColor);
    // Draw white frame if the main color matches the panel backgound
    if (currentMainColor == GRAY)
        {
            for(int x = 0; x < 62; x++)
            {
                drawpixel(x+startingPixelXOffset, 0 + startingPixelYOffset, WHITE);
                drawpixel(x+startingPixelXOffset, 29 + startingPixelYOffset, WHITE);
            }
            for(int y = 0; y < 30; y++)
            {
                drawpixel(0+startingPixelXOffset, y + startingPixelYOffset, WHITE);
                drawpixel(61+startingPixelXOffset, y + startingPixelYOffset, WHITE);
            }
        }
}

// ----------------------------------------------------
// Paint
// ----------------------------------------------------
void R_Paint(int x1, int y1, int x2, int y2) {
    // Draw initial point
    if (bushSize <= 1) {
        if (x1 < SCREEN_WIDTH && y1 < SCREEN_HEIGHT) {
            drawpixel(x1, y1, drawing ? currentMainColor : currentAltColor);
        }
    } else {
        R_DrawCircle(x1, y1, bushSize);
    }

    // Bresenham's line algorithm for efficient line drawing
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (x1 != x2 || y1 != y2) {
        if (bushSize <= 1) {
            if (x1 < SCREEN_WIDTH && y1 < SCREEN_HEIGHT) {
                drawpixel(x1, y1, drawing ? currentMainColor : currentAltColor);
            }
        } else {
            R_DrawCircle(x1, y1, bushSize);
        }

        int e2 = 2 * err;
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
void R_DrawCircle(int x0, int y0, int r)
{
    if(r > 1) {
        for(int y=-r; y<=r; y++)
            for(int x=-r; x<=r; x++)
                if((2*x+1)*(2*x+1) + (2*y+1)*(2*y+1) <= 4*r*r)
                    if(x0+x >= 0 && x0+x < SCREEN_WIDTH && y0+y >= 0 && y0+y < SCREEN_HEIGHT)
                        drawpixel(x0+x, y0+y, drawing ? currentMainColor : currentAltColor);
    } else {
        if(x0 < SCREEN_WIDTH && y0 < SCREEN_HEIGHT)
            drawpixel(x0, y0, drawing ? currentMainColor : currentAltColor);
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
            if(leftestX < SCREEN_WIDTH && readpixel(leftestX, curElement.y) == ogColor)
            {
                //pixels[leftestX + curElement.y * width] = color;
                drawpixel(leftestX, curElement.y, color);

                // Check above this pixel
                if (alreadyCheckedBelow == false && (curElement.y-1) >= 0
                    && (curElement.y-1) < SCREEN_HEIGHT
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
                    && (curElement.y+1) < SCREEN_HEIGHT
                    && readpixel(leftestX, curElement.y+1) == ogColor)
                    {
                        // If we never checked it, add it to the stack
                        FF_StackPush(stack, leftestX, curElement.y+1, &stackTop);
                        alreadyCheckedAbove = true;
                    }
                else if (alreadyCheckedAbove == true
                    && (curElement.y+1) < SCREEN_WIDTH
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
