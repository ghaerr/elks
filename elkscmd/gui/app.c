#include <stdlib.h>
#include "app.h"
#include "input.h"
#include "render.h"
#include "gui.h"
#include "event.h"
#include "graphics.h"
#include "mouse.h"

#define LIBPATH     "/lib/"     // path to images

// --------------------------------------------
// Definition of Globals
// --------------------------------------------
app_t DrawingApp;

boolean_t drawing;              // Are we drawing with the left mouse button?
boolean_t altdrawing;           // Are we drawing with the right mouse button?
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

    // for(int y = 0; y < SCREEN_HEIGHT; y++)
    //     for(int x = 0; x < SCREEN_WIDTH; x++)
    //         drawpixel(x, y, BLACK);

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
#if UNUSED
    paletteButtons[0].name = "BrightnessSelector";
    paletteButtons[0].box.x = 775;
    paletteButtons[0].box.y = 10;
    paletteButtons[0].box.w = 16;
    paletteButtons[0].box.h = 64;
    paletteButtons[0].OnClick = G_BrightnessButtonOnClick;
    paletteButtons[0].render = false;
#endif
    paletteButtons[1].name = "ColorPicker";
    paletteButtons[1].box.x = SCREEN_WIDTH + 10;
    paletteButtons[1].box.y = 10;
    paletteButtons[1].box.w = 128;
    paletteButtons[1].box.h = 128;
    paletteButtons[1].OnClick = G_ColorPickerOnClick;
    paletteButtons[1].render = false;

    // Bush Sizes 1
    paletteButtons[2].name = "BushSize1";
    paletteButtons[2].box.x = SCREEN_WIDTH + 11 + 32*0;
    paletteButtons[2].box.y = 195;
    paletteButtons[2].box.w = 30;
    paletteButtons[2].box.h = 30;
    paletteButtons[2].OnClick = G_SetBushSize;
    paletteButtons[2].data1 = 1;
    paletteButtons[2].render = true;

    // Bush Sizes 2
    paletteButtons[3].name = "BushSize2";
    paletteButtons[3].box.x = SCREEN_WIDTH + 11 + 32*1;
    paletteButtons[3].box.y = 195;
    paletteButtons[3].box.w = 30;
    paletteButtons[3].box.h = 30;
    paletteButtons[3].OnClick = G_SetBushSize;
    paletteButtons[3].data1 = 2;
    paletteButtons[3].render = true;

    // Bush Sizes 3
    paletteButtons[4].name = "BushSize3";
    paletteButtons[4].box.x = SCREEN_WIDTH + 11 + 32*2;
    paletteButtons[4].box.y = 195;
    paletteButtons[4].box.w = 30;
    paletteButtons[4].box.h = 30;
    paletteButtons[4].OnClick = G_SetBushSize;
    paletteButtons[4].data1 = 3;
    paletteButtons[4].render = true;

    // Bush Sizes 4
    paletteButtons[5].name = "BushSize4";
    paletteButtons[5].box.x = SCREEN_WIDTH + 11 + 32*3;
    paletteButtons[5].box.y = 195;
    paletteButtons[5].box.w = 30;
    paletteButtons[5].box.h = 30;
    paletteButtons[5].OnClick = G_SetBushSize;
    paletteButtons[5].data1 = 4;
    paletteButtons[5].render = true;

    // Bush Sizes 5
    paletteButtons[6].name = "BushSize5";
    paletteButtons[6].box.x = SCREEN_WIDTH + 11 + 32*0;
    paletteButtons[6].box.y = 227;
    paletteButtons[6].box.w = 30;
    paletteButtons[6].box.h = 30;
    paletteButtons[6].OnClick = G_SetBushSize;
    paletteButtons[6].data1 = 6;
    paletteButtons[6].render = true;

    // Bush Sizes 6
    paletteButtons[7].name = "BushSize6";
    paletteButtons[7].box.x = SCREEN_WIDTH + 11 + 32*1;
    paletteButtons[7].box.y = 227;
    paletteButtons[7].box.w = 30;
    paletteButtons[7].box.h = 30;
    paletteButtons[7].OnClick = G_SetBushSize;
    paletteButtons[7].data1 = 8;
    paletteButtons[7].render = true;

    // Bush Sizes 7
    paletteButtons[8].name = "BushSize7";
    paletteButtons[8].box.x = SCREEN_WIDTH + 11 + 32*2;
    paletteButtons[8].box.y = 227;
    paletteButtons[8].box.w = 30;
    paletteButtons[8].box.h = 30;
    paletteButtons[8].OnClick = G_SetBushSize;
    paletteButtons[8].data1 = 10;
    paletteButtons[8].render = true;

    // Bush Sizes 8
    paletteButtons[9].name = "BushSize8";
    paletteButtons[9].box.x = SCREEN_WIDTH + 11 + 32*3;
    paletteButtons[9].box.y = 227;
    paletteButtons[9].box.w = 30;
    paletteButtons[9].box.h = 30;
    paletteButtons[9].OnClick = G_SetBushSize;
    paletteButtons[9].data1 = 12;
    paletteButtons[9].render = true;

    // Save
    paletteButtons[10].name = "SaveButton";
    paletteButtons[10].box.x = SCREEN_WIDTH + 10;
    paletteButtons[10].box.y = 300;
    paletteButtons[10].box.w = 32;
    paletteButtons[10].box.h = 32;
    paletteButtons[10].OnClick = G_SaveButtonOnClick;
    paletteButtons[10].render = true;
    paletteButtons[10].fileName = LIBPATH "save.bmp";

    // Clear Screen
    paletteButtons[11].name = "Cls";
    paletteButtons[11].box.x = SCREEN_WIDTH + 10 + 32*2;
    paletteButtons[11].box.y = 300;
    paletteButtons[11].box.w = 32;
    paletteButtons[11].box.h = 32;
    paletteButtons[11].OnClick = G_ClearScreen;
    paletteButtons[11].render = true;
    paletteButtons[11].fileName = LIBPATH "cls.bmp";

    // Quit
    paletteButtons[12].name = "QuitButton";
    paletteButtons[12].box.x = SCREEN_WIDTH + 10 + 32*3;
    paletteButtons[12].box.y = 300;
    paletteButtons[12].box.w = 32;
    paletteButtons[12].box.h = 32;
    paletteButtons[12].OnClick = G_QuitButtonOnClick;
    paletteButtons[12].render = true;
    paletteButtons[12].fileName = LIBPATH "quit.bmp";
}


// --------------------------------------------
// Update
// --------------------------------------------
void A_GameLoop(void)
{
    movecursor(mx, my);

    // In canvas
    if(mx <= SCREEN_WIDTH)
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
