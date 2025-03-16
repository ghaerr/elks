#include <stdlib.h>
#include "app.h"
#include "render.h"
#include "gui.h"
#include "graphics.h"

#if UNUSED
int G_BrightnessButtonOnClick(struct button_s* btn)
{
    //Transform mouse relative to box
    int relativeY = my-btn->box.y;
    paletteBrightness = relativeY * 4;

    R_UpdateColorPicker();
    return 0;
}
#endif

int G_ColorPickerOnClick(struct button_s* btn)
{
    // Read and store the pixel selected by the mouse
    currentMainColor = readpixel(mx, my);
    R_DrawCurrentColor();
    return 0;
}

int G_SaveButtonOnClick(struct button_s* btn)
{
    // Save drawing area as bmp
    if (save_bmp("export.bmp"))
        __dprintf("Error writing image 'export.bmp'\n");
    __dprintf("Image exported as 'export.bmp'\n");
    return 0;
}

int G_QuitButtonOnClick(struct button_s* btn)
{
    DrawingApp.quit = true;
    return 0;
}

int G_SetBushSize(struct button_s* btn)
{
    bushSize = btn->data1;
    return bushSize;
}

int G_ClearScreen(struct button_s* btn)
{
    R_ClearCanvas();
    return 0;
}
