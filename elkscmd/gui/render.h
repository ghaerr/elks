#ifndef RENDERING_H_INCLUDED
#define RENDERING_H_INCLUDED

// ----------------------------------------------------
// Clear the screen
// ----------------------------------------------------
void R_ClearCanvas(void);

// ----------------------------------------------------
// Draws the Palette
// ----------------------------------------------------
void R_DrawPalette(void);

// ----------------------------------------------------
// Paint
// ----------------------------------------------------
void R_Paint(int x1, int y1, int x2, int y2);

// ----------------------------------------------------
// Updates the Color Picker when brightness changes.
// ----------------------------------------------------
void R_UpdateColorPicker(void);

// ----------------------------------------------------
// Updates the Current Color when changes.
// ----------------------------------------------------
void R_DrawCurrentColor(void);

// ----------------------------------------------------
// Used to draw at 2+px bush size.
// ----------------------------------------------------
void R_DrawDisk(int x0, int y0, int r, int color, int X_lim);
void R_DrawCircle(int cx, int cy, int r, int color);
void R_DrawRectangle(int x1, int y1, int x2, int y2);
void R_DrawFilledRectangle(int x1, int y1, int x2, int y2);

// ----------------------------------------------------
// Draws all the Palette Buttons
// ----------------------------------------------------
void R_DrawAllButtons(void);
void R_HighlightActiveButton(void);

// ----------------------------------------------------
// Draws Brush Buttons
// ----------------------------------------------------
void DrawButtonCircle(int x0, int y0, int w, int h, int r);

// ----------------------------------------------------
// Flood Fill Stack
// ----------------------------------------------------
void R_FloodFillStack(int x, int y, int newColor, int targetColor);
int R_FrontFill(int x0, int y0, int newColor, int targetColor);

// ----------------------------------------------------
// Line Flood Fill, for the bucket tool
// ----------------------------------------------------
void R_LineFloodFill(int x, int y, int color, int ogColor);

// ----------------------------------------------------
// Converts from HSV to RGB
// ----------------------------------------------------
ColorRGB_t HSVtoRGB(ColorHSV_t colorHSV);
#endif
