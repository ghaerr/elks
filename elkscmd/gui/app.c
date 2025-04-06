#include <stdlib.h>
#include "app.h"
#include "input.h"
#include "render.h"
#include "gui.h"
#include "event.h"
#include "graphics.h"
#include "mouse.h"

// --------------------------------------------
// Definition of Globals
// --------------------------------------------
app_t DrawingApp;

boolean_t drawing;              // Are we drawing with the left mouse button?
boolean_t altdrawing;           // Are we drawing with the right mouse button?
boolean_t floodFill;            // Are we flood filling with the left mouse button?
boolean_t floodFillCalled;      // True if user is trying to floodfill
int mx,my;                      // Mouse X and Y
int omx, omy;                   // Old MouseX and Old MouseY (pos at previous update)
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

    A_InitPalette();
}


// --------------------------------------------
// Initializes the palette
// --------------------------------------------
void A_InitPalette(void)
{
    // Set defaults
    currentMainColor = WHITE;
    currentAltColor = BLACK;

    paletteBrightness = 256;
    bushSize = 3;


    // Create and configure buttons
    // --- Helper macro to initialize buttons ---
#define INIT_BUTTON(ID, LABEL, X, Y, W, H, CLICK, RENDER) \
    paletteButtons[ID].name = LABEL; \
    paletteButtons[ID].box.x = (X); \
    paletteButtons[ID].box.y = (Y); \
    paletteButtons[ID].box.w = (W); \
    paletteButtons[ID].box.h = (H); \
    paletteButtons[ID].OnClick = CLICK; \
    paletteButtons[ID].render = RENDER;

    #if UNUSED
        INIT_BUTTON(0, "BrightnessSelector", 775, 10, 16, 64, G_BrightnessButtonOnClick, 0);
    #endif

    // --- Color Picker ---
    INIT_BUTTON(1, "ColorPicker", CANVAS_WIDTH + 10, 10, 128, 128, G_ColorPickerOnClick, 0);

    // --- Brush Size Buttons (8 total in two rows) ---
    {
        int brushSizes[8] = {1, 2, 3, 4,
                                   6, 8, 10, 12};
        char* brushNames[8] = {
            "BushSize1", "BushSize2", "BushSize3", "BushSize4",
            "BushSize5", "BushSize6", "BushSize7", "BushSize8"
        };

        for (int i = 0; i < 8; ++i) {
            int index = i + 2;
            int col = i % 4;
            int row = i / 4;
            INIT_BUTTON(index,
                        brushNames[i],
                        CANVAS_WIDTH + 11 + 32 * col,
                        195 + row * 32,
                        30, 30,
                        G_SetBushSize, 1);
            paletteButtons[index].data1 = brushSizes[i];
        }
    }

    // --- Utility Buttons ---
    {
        int utilCount = 5;
        int indices[]   = {10, 11, 12, 13, 14};
        char* names[]   = {"SaveButton", "FillButton", "BrushButton", "QuitButton", "Cls"};
        int offsetX[]   = {0,  32,   0, 32 * 3, 32 * 3};
        int offsetY[]   = {0, -32, -32,      0,    -32};
        int (*handlers[])(struct button_s*) = {
            G_SaveButtonOnClick,
            G_SetFloodFill,
            G_SetBrush,
            G_QuitButtonOnClick,
            G_ClearScreen
        };
        char* files[] = {
            LIBPATH "save.bmp",
            LIBPATH "fill.bmp",
            LIBPATH "brush.bmp",
            LIBPATH "quit.bmp",
            LIBPATH "cls.bmp"
        };

        for (int i = 0; i < utilCount; ++i) {
            INIT_BUTTON(indices[i],
                        names[i],
                        CANVAS_WIDTH + 10 + offsetX[i],
                        300 + offsetY[i],
                        32, 32,
                        handlers[i],
                        1);
            paletteButtons[indices[i]].fileName = files[i];
        }
    }

#undef INIT_BUTTON
}


// --------------------------------------------
// Update
// --------------------------------------------
void A_GameLoop(void)
{
    movecursor(mx, my);

    if((drawing || altdrawing) && mx > CANVAS_WIDTH) mx = CANVAS_WIDTH;

    // In canvas
    if(mx <= CANVAS_WIDTH)
    {
        mouseOnPalette = false;

        if(drawing || altdrawing)
        {
            R_Paint(omx, omy, mx, my);
        }
        if(floodFillCalled == true)
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
