#include <stdlib.h>
#include <string.h>
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

int mx,my;                      // Mouse X and Y
int omx, omy;                   // Old MouseX and Old MouseY (pos at previous update)
int startX, startY;             // Circle center X and Y
int lastRadius;                 // Circle old radius
boolean_t mouseOnPalette;       // True if the mouse is on the palette and not the canvas
int paletteBrightness;          // The brightness of the color picker
int bushSize;                   // Size of the brush
int currentMainColor;           // The selected color for LMB
int currentAltColor;            // Color for RMB (eraser)
int current_color;
int currentModeButton;
DrawingMode current_mode;
DrawingState current_state;
boolean_t AltFinalize;

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

    currentModeButton = 10; // BrushButton
    current_mode = mode_Brush;
    current_state = state_Idle;
    startX = 0;
    startY = 0;
    lastRadius = 0;
    AltFinalize = false;

    A_InitPalette();
}


// --------------------------------------------
// Initializes the palette
// --------------------------------------------

// --- Helper macro to initialize buttons ---
#define DEFINE_BUTTON(ID, NAME, X, Y, W, H, CLICK, RENDER) \
    paletteButtons[ID].name = NAME; \
    paletteButtons[ID].id   = ID;   \
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
    DEFINE_BUTTON_WITH_FILE(10, "BrushButton", CANVAS_WIDTH + 10 + 32*0, FUNC_Y-32, 32, 32, G_SetBrush,          1, LIBPATH "brush.bmp");
    DEFINE_BUTTON_WITH_FILE(11, "FillButton",  CANVAS_WIDTH + 10 + 32*1, FUNC_Y-32, 32, 32, G_SetFloodFill,      1, LIBPATH "fill.bmp");
    DEFINE_BUTTON_WITH_FILE(12, "CircleButton",CANVAS_WIDTH + 10 + 32*2, FUNC_Y-32, 32, 32, G_SetCircle,         1, LIBPATH "circle.bmp");
    DEFINE_BUTTON_WITH_FILE(13, "RectangleBtn",CANVAS_WIDTH + 10 + 32*3, FUNC_Y-32, 32, 32, G_SetRectangle,      1, LIBPATH "rectangle.bmp");
    DEFINE_BUTTON_WITH_FILE(14, "SaveButton",  CANVAS_WIDTH + 10 + 32*0, FUNC_Y,    32, 32, G_SaveButtonOnClick, 1, LIBPATH "save.bmp");
    DEFINE_BUTTON_WITH_FILE(15, "Cls",         CANVAS_WIDTH + 10 + 32*2, FUNC_Y,    32, 32, G_ClearScreen,       1, LIBPATH "cls.bmp");
    DEFINE_BUTTON_WITH_FILE(16, "QuitButton",  CANVAS_WIDTH + 10 + 32*3, FUNC_Y,    32, 32, G_QuitButtonOnClick, 1, LIBPATH "quit.bmp");
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

    if((current_state == state_Drawing) && mx > CANVAS_WIDTH) mx = CANVAS_WIDTH;

    if(current_state == state_Finalize)
    {
        switch (current_mode) {
            case mode_Brush:
            break;

            case mode_Fill:
                if(mx <= CANVAS_WIDTH){
                    hidecursor();
                    R_LineFloodFill(omx, omy, current_color, readpixel(mx, my));
                    showcursor();
                    current_state = state_Idle;
                }
            break;

            case mode_Circle:
                hidecursor();
                // Erase the old preview circle
                if (lastRadius > 0) {
                    set_op(0x18);    // turn on XOR drawing
                    R_DrawCircle(startX, startY, lastRadius, current_color); // erase using XOR
                }
                int radius = distance(startX, startY, omx, omy);
                set_op(0);       // turn off XOR drawing
                if (AltFinalize)
                    R_DrawDisk(startX, startY, radius, current_color, CANVAS_WIDTH);
                else
                    R_DrawCircle(startX, startY, radius, current_color); // Final draw
                showcursor();
                lastRadius = 0;
                current_state = state_Idle;
            break;
            case mode_Rectangle:
                hidecursor();
                // Erase the old preview
                set_op(0x18);    // turn on XOR drawing
                R_DrawRectangle(startX, startY, omx, omy); // erase using XOR
                set_op(0);       // turn off XOR drawing
                mx = (mx > CANVAS_WIDTH) ? CANVAS_WIDTH : mx;
                if (AltFinalize)
                    R_DrawFilledRectangle(startX, startY, mx, my); // Final draw
                else
                    R_DrawRectangle(startX, startY, mx, my);
                showcursor();
                current_state = state_Idle;
            break;
        }
    }

    // In canvas
    if(mx <= CANVAS_WIDTH)
    {
        mouseOnPalette = false;

        if(current_state == state_Drawing){
            switch (current_mode) {
                case mode_Brush:
                    R_Paint(omx, omy, mx, my);
                break;

                case mode_Fill:
                break;

                case mode_Circle:
                    // Erase the old preview circle
                    if (lastRadius > 0) {
                        R_DrawCircle(startX, startY, lastRadius, current_color); // XOR again to erase
                    }
                    // Draw new preview
                    int radius = distance(startX, startY, mx, my);
                    R_DrawCircle(startX, startY, radius, current_color);
                    lastRadius = radius;
                break;

                case mode_Rectangle:
                    // Erase the old preview
                    R_DrawRectangle(startX, startY, omx, omy); // XOR again to erase
                    // Draw new preview
                    R_DrawRectangle(startX, startY, mx, my);
                break;
            }
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
//  A_ [Application/Implementation Specific]
//  G_ [GUI]
//  I_ [Input/Implementation Specific]
//  R_ [Rendering]
//  U_ [Utilities]
// ------------------------------------------------

int main(int argc, char* argv[])
{
    // Check if one filename was provided
    if (argc > 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s <bmp_filename>\n", argv[0]);
        return 1;
    }
    else if (argc > 1) {
        // Check if the file has a .bmp extension
        char* filename = argv[1];
        char* extension = strrchr(filename, '.');
        if (!extension || strcmp(extension, ".bmp") != 0) {
            printf("Error: File must have .bmp extension\n");
            return 1;
        }
        // Init Application
        A_InitTomentPainter();
        // Load the BMP file
        draw_bmp(filename, 0, 0);
    }
    else {
        // Init Application
        A_InitTomentPainter();
    }

    // Draw Palette
    R_DrawPalette();

    // Highlight Brush Button
    R_HighlightActiveButton();

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
