#ifndef GUIBUTTONS_H_INCLUDED
#define GUIBUTTONS_H_INCLUDED

// -------------------------------
// Callbacks for each GUIButton
// -------------------------------

int G_BrightnessButtonOnClick(struct button_s* btn);
int G_ColorPickerOnClick(struct button_s* btn);
int G_SaveButtonOnClick(struct button_s* btn);
int G_QuitButtonOnClick(struct button_s* btn);
int G_SetBushSize(struct button_s* btn);
int G_ClearScreen(struct button_s* btn);
int G_SetFloodFill(struct button_s* btn);
int G_SetCircle(struct button_s* btn);
int G_SetRectangle(struct button_s* btn);
int G_SetBrush(struct button_s* btn);


#endif
