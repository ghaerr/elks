#ifndef APPLICATION_H_INCLUDED
#define APPLICATION_H_INCLUDED

// Drawing + Palette
#define SCREEN_WIDTH    (SCREENWIDTH-PALETTE_WIDTH)
#define SCREEN_HEIGHT   SCREENHEIGHT

#define PALETTE_WIDTH 148

// Boolean Data Type
typedef enum boolean_e
{
    false = 0,
    true = 1
} boolean_t;

// Represents the colors in HSV (for Color Picker)
typedef struct ColorHSV_s
{
    int h, s, v;
} ColorHSV_t;

// Represents the colors in RGB (for Color Picker)
typedef struct ColorRGB_s
{
    int r,g,b,a;
} ColorRGB_t;

typedef struct rect
{
    int x, y;
    int w, h;
} rect;

// GUI Button Data Type
typedef struct button_s
{
    char* name;
    rect box;
    int (*OnClick)(struct button_s* btn);
    int data1;
    boolean_t render;
    char* fileName;
} button_t;

// Represents a pixel
typedef struct transform2d_x
{
    int x,y;
} transform2d_t;

typedef struct app_s
{
    boolean_t quit;
} app_t;

extern app_t DrawingApp;

extern boolean_t drawing;              // Are we drawing with the left mouse button?
extern boolean_t altdrawing;           // Are we drawing with the right mouse button?
extern boolean_t floodFillCalled;      // True if user is trying to flood fiil
extern int mx,my;                      // Mouse X and Y
extern int omx, omy;                   // Old MouseX and MouseY (pos at previous update)
extern boolean_t mouseOnPalette;       // True if the mouse is on the palette and not the canvas

extern int paletteBrightness;          // The brightness of the color picker
extern int bushSize;                   // Size of the brush
extern int currentMainColor;           // The selected color for LMB
extern int currentAltColor;            // Color for RMB (eraser)

// Palette Buttons
#define PALETTE_BUTTONS_COUNT 13 //14

// All the buttons
extern button_t paletteButtons[PALETTE_BUTTONS_COUNT];


// -------------------------------
// Initializes the program
// -------------------------------
void A_InitTomentPainter(void);

// -------------------------------
// Initializes the palette (right toolbar)
// -------------------------------
void A_InitPalette(void);

// -------------------------------
// Update
// -------------------------------
void A_GameLoop(void);

#endif
