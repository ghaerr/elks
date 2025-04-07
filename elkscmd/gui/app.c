#include <stdlib.h>
#include "app.h"
#include "input.h"
#include "render.h"
#include "gui.h"
#include "event.h"
#include "graphics.h"
#include "vgalib.h"
#include "mouse.h"

// --------------------------------------------
// Definition of Globals
// --------------------------------------------
app_t DrawingApp;

boolean_t drawing;              // Are we drawing with the left mouse button?
boolean_t altdrawing;           // Are we drawing with the right mouse button?
boolean_t floodFill;            // Are we flood filling with the left mouse button?
boolean_t floodFillCalled;      // True if user is trying to floodfill
boolean_t circleMode;        // Are we drawing circle with the left mouse button?
boolean_t circleDrawing;
boolean_t circleDrawingCalled;  // True if user is trying to draw a circle
int mx,my;                      // Mouse X and Y
int omx, omy;                   // Old MouseX and Old MouseY (pos at previous update)
int omx, omy;                   // Old MouseX and Old MouseY (pos at previous update)
int startX, startY;             // Circle center X and Y
int lastRadius;                 // Circle old radius
boolean_t mouseOnPalette;       // True if the mouse is on the palette and not the canvas
int paletteBrightness;          // The brightness of the color picker
int bushSize;                   // Size of the brush
int currentMainColor;           // The selected color for LMB
int currentAltColor;            // Color for RMB (eraser)

button_t paletteButtons[PALETTE_BUTTONS_COUNT];

// --------------------------------------------
// Initializes the program
// --------------------------------------------
void A_InitTomentPainter(void)
{
    if (event_open() < 0) {
        __dprintf("Can't init events/mouse\n");
        return;
    }
    int EGA = (int)getenv("EGAMODE");
    graphics_open(EGA? VGA_640x350x16: VGA_640x480x16);

    DrawingApp.quit = false;
    floodFill = false;
    circleMode = false;
    circleDrawing = false;
    lastRadius = 0;

    A_InitPalette();
}


// --------------------------------------------
// Initializes the palette
// --------------------------------------------

// --- Helper macro to initialize buttons ---
#define DEFINE_BUTTON(ID, NAME, X, Y, W, H, CLICK, RENDER) \
    paletteButtons[ID].name = NAME; \
    paletteButtons[ID].box.x = (X); \
    paletteButtons[ID].box.y = (Y); \
    paletteButtons[ID].box.w = (W); \
    paletteButtons[ID].box.h = (H); \
    paletteButtons[ID].OnClick = CLICK; \
    paletteButtons[ID].render = RENDER;

#define DEFINE_BUTTON_WITH_DATA(ID, NAME, X, Y, W, H, CLICK, RENDER, DATA) \
    DEFINE_BUTTON(ID, NAME, X, Y, W, H, CLICK, RENDER) \
    paletteButtons[ID].data1 = DATA;

#define DEFINE_BUTTON_WITH_FILE(ID, NAME, X, Y, W, H, CLICK, RENDER, FILE) \
    DEFINE_BUTTON(ID, NAME, X, Y, W, H, CLICK, RENDER) \
    paletteButtons[ID].fileName = FILE;


void A_InitPalette(void)
{
    // Set defaults
    currentMainColor = WHITE;
    currentAltColor = BLACK;

    paletteBrightness = 256;
    bushSize = 3;

    // Create and configure buttons
    #if UNUSED
        DEFINE_BUTTON(0, "BrightnessSelector", 775, 10, 16, 64, G_BrightnessButtonOnClick, 0);
    #endif

    // Color Picker
    DEFINE_BUTTON(1, "ColorPicker", CANVAS_WIDTH + 10, 10, 128, 128, G_ColorPickerOnClick, 0);

    // Brush Sizes (2 rows, 4 buttons each)
    #define BRUSH_Y 195
    DEFINE_BUTTON_WITH_DATA(2, "BushSize1", CANVAS_WIDTH + 11 + 32*0, BRUSH_Y, 30, 30, G_SetBushSize, 1, 1);
    DEFINE_BUTTON_WITH_DATA(3, "BushSize2", CANVAS_WIDTH + 11 + 32*1, BRUSH_Y, 30, 30, G_SetBushSize, 1, 2);
    DEFINE_BUTTON_WITH_DATA(4, "BushSize3", CANVAS_WIDTH + 11 + 32*2, BRUSH_Y, 30, 30, G_SetBushSize, 1, 3);
    DEFINE_BUTTON_WITH_DATA(5, "BushSize4", CANVAS_WIDTH + 11 + 32*3, BRUSH_Y, 30, 30, G_SetBushSize, 1, 4);
    DEFINE_BUTTON_WITH_DATA(6, "BushSize5", CANVAS_WIDTH + 11 + 32*0, BRUSH_Y+32, 30, 30, G_SetBushSize, 1, 6);
    DEFINE_BUTTON_WITH_DATA(7, "BushSize6", CANVAS_WIDTH + 11 + 32*1, BRUSH_Y+32, 30, 30, G_SetBushSize, 1, 8);
    DEFINE_BUTTON_WITH_DATA(8, "BushSize7", CANVAS_WIDTH + 11 + 32*2, BRUSH_Y+32, 30, 30, G_SetBushSize, 1, 10);
    DEFINE_BUTTON_WITH_DATA(9, "BushSize8", CANVAS_WIDTH + 11 + 32*3, BRUSH_Y+32, 30, 30, G_SetBushSize, 1, 12);

    // Functional buttons with icons
    #define FUNC_Y 300
    DEFINE_BUTTON_WITH_FILE(10, "SaveButton",  CANVAS_WIDTH + 10 + 32*0, FUNC_Y,    32, 32, G_SaveButtonOnClick, 1, LIBPATH "save.bmp");
    DEFINE_BUTTON_WITH_FILE(11, "FillButton",  CANVAS_WIDTH + 10 + 32*1, FUNC_Y-32, 32, 32, G_SetFloodFill,      1, LIBPATH "fill.bmp");
    DEFINE_BUTTON_WITH_FILE(12, "BrushButton", CANVAS_WIDTH + 10 + 32*0, FUNC_Y-32, 32, 32, G_SetBrush,          1, LIBPATH "brush.bmp");
    DEFINE_BUTTON_WITH_FILE(13, "CircleButton",CANVAS_WIDTH + 10 + 32*2, FUNC_Y-32, 32, 32, G_SetCircle,         1, LIBPATH "circle.bmp");
    DEFINE_BUTTON_WITH_FILE(14, "QuitButton",  CANVAS_WIDTH + 10 + 32*3, FUNC_Y,    32, 32, G_QuitButtonOnClick, 1, LIBPATH "quit.bmp");
    DEFINE_BUTTON_WITH_FILE(15, "Cls",         CANVAS_WIDTH + 10 + 32*3, FUNC_Y-32, 32, 32, G_ClearScreen,       1, LIBPATH "cls.bmp");
}

// Utility: distance between two points
// Compute the minimal distance along x or y (Chebyshev distance)
static int distance(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    return (dx > dy) ? dx : dy;  // Return the maximum of dx or dy
}

// --------------------------------------------
// Update
// --------------------------------------------
void A_GameLoop(void)
{
    movecursor(mx, my);

    if((drawing || altdrawing || circleDrawing) && mx > CANVAS_WIDTH) mx = CANVAS_WIDTH;

    if(circleDrawingCalled)
    {
        circleDrawingCalled = false;

        // Erase the old preview circle
        if (lastRadius > 0) {
            set_op(0x18);    // turn on XOR drawing
            R_DrawCircle(startX, startY, lastRadius, currentMainColor); // erase using XOR
        }
        int radius = distance(startX, startY, omx, omy);
        set_op(0);       // turn off XOR drawing
        R_DrawCircle(startX, startY, radius, currentMainColor); // Final draw
        lastRadius = 0;
    }

    // In canvas
    if(mx <= CANVAS_WIDTH)
    {
        mouseOnPalette = false;

        if(drawing || altdrawing)
        {
            R_Paint(omx, omy, mx, my);
        }
        if(circleDrawing)
        {
            int radius = distance(startX, startY, mx, my);
            // Erase the old preview circle
            if (lastRadius > 0) {
                R_DrawCircle(startX, startY, lastRadius, currentMainColor); // XOR again to erase
            }

            // Draw new preview
            R_DrawCircle(startX, startY, radius, currentMainColor);
            lastRadius = radius;
        }
        if(floodFillCalled)
        {
            floodFillCalled = false;
            hidecursor();
            R_LineFloodFill(omx, omy, currentMainColor, readpixel(mx, my));
            showcursor();
        }
    }
    else // In toolbar
    {
        mouseOnPalette = true;
    }
}

// ------------------------------------------------
//  Source Code
// ------------------------------------------------
//  A_ [ Application/Implementation Specific]
//  G_ [GUI]
//  I_ [Input/Implementation Specific]
//  R_ [Rendering]
//  U_ [Utilities]
// ------------------------------------------------

int main(int argc, char* argv[])
{
    // Init Application
    A_InitTomentPainter();

    // Draw Palette
    R_DrawPalette();

    initcursor();

    // GameLoop
    while(DrawingApp.quit == false)
    {
        I_HandleInput();

        A_GameLoop();
    }
    graphics_close();
    return 0;
}
