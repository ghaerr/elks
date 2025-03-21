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

#endif
