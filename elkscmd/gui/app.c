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
    graphics_open(VGA_640x480x16);

    DrawingApp.quit = false;
    
    for(int y = 0; y < SCREEN_HEIGHT; y++)
        for(int x = 0; x < SCREEN_WIDTH; x++)
            drawpixel(x, y, BLACK);

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
    paletteButtons[1].box.x = SCREEN_WIDTH + 7 + 8;
    paletteButtons[1].box.y = 10 + 16;
    paletteButtons[1].box.w = 128;
    paletteButtons[1].box.h = 32;
    paletteButtons[1].OnClick = G_ColorPickerOnClick;
    paletteButtons[1].render = false;

    paletteButtons[2].name = "SaveButton";
    paletteButtons[2].box.x = SCREEN_WIDTH + (PALETTE_WIDTH / 3) - 25;
    paletteButtons[2].box.y = 400;
    paletteButtons[2].box.w = 50;
    paletteButtons[2].box.h = 50;
    paletteButtons[2].OnClick = G_SaveButtonOnClick;
    paletteButtons[2].render = true;
    paletteButtons[2].fileName = LIBPATH "save.bmp";

    // Bush Sizes 1
    paletteButtons[3].name = "BushSize1";
    paletteButtons[3].box.x = SCREEN_WIDTH + 5;
    paletteButtons[3].box.y = 250;
    paletteButtons[3].box.w = 25;
    paletteButtons[3].box.h = 25;
    paletteButtons[3].OnClick = G_SetBushSize;
    paletteButtons[3].data1 = 1;
    paletteButtons[3].render = true;
    paletteButtons[3].fileName = LIBPATH "bs1.bmp";

    // Bush Sizes 2
    paletteButtons[4].name = "BushSize2";
    paletteButtons[4].box.x = SCREEN_WIDTH + 35;
    paletteButtons[4].box.y = 250;
    paletteButtons[4].box.w = 25;
    paletteButtons[4].box.h = 25;
    paletteButtons[4].OnClick = G_SetBushSize;
    paletteButtons[4].data1 = 2;
    paletteButtons[4].render = true;
    paletteButtons[4].fileName = LIBPATH "bs2.bmp";

    // Bush Sizes 3
    paletteButtons[5].name = "BushSize3";
    paletteButtons[5].box.x = SCREEN_WIDTH + 65;
    paletteButtons[5].box.y = 250;
    paletteButtons[5].box.w = 25;
    paletteButtons[5].box.h = 25;
    paletteButtons[5].OnClick = G_SetBushSize;
    paletteButtons[5].data1 = 3;
    paletteButtons[5].render = true;
    paletteButtons[5].fileName = LIBPATH "bs3.bmp";

    // Bush Sizes 4
    paletteButtons[6].name = "BushSize4";
    paletteButtons[6].box.x = SCREEN_WIDTH + 95;
    paletteButtons[6].box.y = 250;
    paletteButtons[6].box.w = 25;
    paletteButtons[6].box.h = 25;
    paletteButtons[6].OnClick = G_SetBushSize;
    paletteButtons[6].data1 = 4;
    paletteButtons[6].render = true;
    paletteButtons[6].fileName = LIBPATH "bs4.bmp";

    // Bush Sizes 5
    paletteButtons[7].name = "BushSize5";
    paletteButtons[7].box.x = SCREEN_WIDTH + 125;
    paletteButtons[7].box.y = 250;
    paletteButtons[7].box.w = 25;
    paletteButtons[7].box.h = 25;
    paletteButtons[7].OnClick = G_SetBushSize;
    paletteButtons[7].data1 = 5;
    paletteButtons[7].render = true;
    paletteButtons[7].fileName = LIBPATH "bs5.bmp";

    // Bush Sizes 6
    paletteButtons[8].name = "BushSize6";
    paletteButtons[8].box.x = SCREEN_WIDTH + 5;
    paletteButtons[8].box.y = 295;
    paletteButtons[8].box.w = 25;
    paletteButtons[8].box.h = 25;
    paletteButtons[8].OnClick = G_SetBushSize;
    paletteButtons[8].data1 = 6;
    paletteButtons[8].render = true;
    paletteButtons[8].fileName = LIBPATH "bs6.bmp";

    // Bush Sizes 7
    paletteButtons[9].name = "BushSize7";
    paletteButtons[9].box.x = SCREEN_WIDTH + 35;
    paletteButtons[9].box.y = 295;
    paletteButtons[9].box.w = 25;
    paletteButtons[9].box.h = 25;
    paletteButtons[9].OnClick = G_SetBushSize;
    paletteButtons[9].data1 = 7;
    paletteButtons[9].render = true;
    paletteButtons[9].fileName = LIBPATH "bs7.bmp";

    // Bush Sizes 8
    paletteButtons[10].name = "BushSize8";
    paletteButtons[10].box.x = SCREEN_WIDTH + 65;
    paletteButtons[10].box.y = 295;
    paletteButtons[10].box.w = 25;
    paletteButtons[10].box.h = 25;
    paletteButtons[10].OnClick = G_SetBushSize;
    paletteButtons[10].data1 = 8;
    paletteButtons[10].render = true;
    paletteButtons[10].fileName = LIBPATH "bs7.bmp";
    
    // Bush Sizes 9
    paletteButtons[11].name = "BushSize9";
    paletteButtons[11].box.x = SCREEN_WIDTH + 95;
    paletteButtons[11].box.y = 295;
    paletteButtons[11].box.w = 25;
    paletteButtons[11].box.h = 25;
    paletteButtons[11].OnClick = G_SetBushSize;
    paletteButtons[11].data1 = 9;
    paletteButtons[11].render = true;
    paletteButtons[11].fileName = LIBPATH "bs8.bmp";

    // Bush Sizes 10
    paletteButtons[12].name = "BushSize10";
    paletteButtons[12].box.x = SCREEN_WIDTH + 125;
    paletteButtons[12].box.y = 295;
    paletteButtons[12].box.w = 25;
    paletteButtons[12].box.h = 25;
    paletteButtons[12].OnClick = G_SetBushSize;
    paletteButtons[12].data1 = 10;
    paletteButtons[12].render = true;
    paletteButtons[12].fileName = LIBPATH "bs8.bmp";

    // Clear Screen
    paletteButtons[13].name = "Cls";
    paletteButtons[13].box.x = SCREEN_WIDTH + 52;
    paletteButtons[13].box.y = 190;
    paletteButtons[13].box.w = 50;
    paletteButtons[13].box.h = 25;
    paletteButtons[13].OnClick = G_ClearScreen;
    paletteButtons[13].render = true;
    paletteButtons[13].fileName = LIBPATH "cls.bmp";

    // Quit
    paletteButtons[14].name = "QuitButton";
    paletteButtons[14].box.x = SCREEN_WIDTH + 2*(PALETTE_WIDTH / 3) + 25 - 40;
    paletteButtons[14].box.y = 400;
    paletteButtons[14].box.w = 39;
    paletteButtons[14].box.h = 50;
    paletteButtons[14].OnClick = G_QuitButtonOnClick;
    paletteButtons[14].render = true;
    paletteButtons[14].fileName = LIBPATH "quit.bmp";

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
