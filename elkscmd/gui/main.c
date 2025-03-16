#include "input.h"
#include "app.h"
#include "render.h"
#include "gui.h"
#include "graphics.h"
#include "mouse.h"

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
