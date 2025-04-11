#ifndef APPLICATION_H_INCLUDED
#define APPLICATION_H_INCLUDED

// Drawing + Palette
#define CANVAS_WIDTH    (SCREENWIDTH-PALETTE_WIDTH)
#define CANVAS_HEIGHT   SCREENHEIGHT

#define PALETTE_WIDTH 148

#define LIBPATH     "/lib/"     // path to images

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
    int id;
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

typedef enum {
    mode_Brush,
    mode_Fill,
    mode_Circle,
    mode_Rectangle
} DrawingMode;

typedef enum {
    state_Idle,        // Not drawing anything
    state_Drawing,     // Actively drawing (mouse left button held)
    state_Finalize     // Releasing mouse, committing shape
} DrawingState;

extern app_t DrawingApp;

extern int mx,my;                      // Mouse X and Y
extern int omx, omy;                   // Old MouseX and MouseY (pos at previous update)
extern int startX, startY;             // Circle center X and Y
extern int lastRadius;                 // Circle old radius
extern boolean_t mouseOnPalette;       // True if the mouse is on the palette and not the canvas

extern DrawingMode current_mode;
extern DrawingState current_state;

extern int paletteBrightness;          // The brightness of the color picker
extern int bushSize;                   // Size of the brush
extern int currentMainColor;           // The selected color for LMB
extern int currentAltColor;            // Color for RMB (eraser)
extern int current_color;
extern int currentModeButton;

// Palette Buttons
#define PALETTE_BUTTONS_COUNT 17

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
