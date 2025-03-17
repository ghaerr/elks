#ifndef INPUT_HANDLING_H_INCLUDED
#define INPUT_HANDLING_H_INCLUDED

#include "event.h"

#define CLAMP(v, min, max)  (v < min) ? min : (v > max) ? max : v

// -------------------------------
// Handles SDL Events and input
// -------------------------------
void I_HandleInput(void);

// -------------------------------
// Checks for GUIButtons
// -------------------------------
void I_HandleGUI(struct event *);
#endif
