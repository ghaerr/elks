#include <stdlib.h>
#include "input.h"
#include "app.h"
#include "render.h"
#include "gui.h"
#include "graphics.h"
#include "vgalib.h"
#include "mouse.h"

static int cursor_sz;

// -------------------------------
// Handles SDL Events and input
// -------------------------------
void I_HandleInput(void)
{
    struct event event;

    if (event_wait_timeout(&event, EV_BLOCK))
    {
        switch (event.type)
        {
            case EVT_QUIT:
                DrawingApp.quit = true;
            break;

            // Get Mouse Pos
            case EVT_MOUSEDOWN:
                // Get latest mouse
                omx = mx;
                omy = my;

                mx = event.x;
                my = event.y;
                if (mx < CANVAS_WIDTH){
                    current_color = currentMainColor;
                    switch (current_mode){
                        case mode_Fill:
                            current_state = state_Finalize;
                            if (event.button == BUTTON_R) current_color = currentAltColor;
                            break;

                        case mode_Circle:
                            current_state = state_Drawing;
                            AltFinalize = (event.button == BUTTON_R);
                            startX = mx;
                            startY = my;
                            lastRadius = 0;
                            hidecursor();
                            set_op(0x18);    // turn on XOR drawing
                            goto break_mousedown;

                        case mode_Rectangle:
                            current_state = state_Drawing;
                            AltFinalize = (event.button == BUTTON_R);
                            startX = mx;
                            startY = my;
                            hidecursor();
                            set_op(0x18);    // turn on XOR drawing
                            goto break_mousedown;

                        default:
                            current_state = state_Drawing;
                            if (event.button == BUTTON_R) current_color = currentAltColor;
                            break;
                    }
                }
                hidecursor();

            break_mousedown:
            break;

            case EVT_MOUSEUP:
                if (current_state == state_Drawing){
                    switch (current_mode){
                        case mode_Circle:
                            current_state = state_Finalize;
                            set_op(0);       // turn off XOR drawing
                            break;

                        case mode_Rectangle:
                            current_state = state_Finalize;
                            set_op(0);       // turn off XOR drawing
                            break;

                        default:
                            current_state = state_Idle;
                            break;
                    }
                }
                showcursor();
            break;

            case EVT_KEYCHAR:
                switch (event.keychar)
                {
                    case 'c':
                        hidecursor();
                        R_ClearCanvas();
                        showcursor();
                    break;

                    case 'e':
                        G_SaveButtonOnClick(NULL);
                    break;

                    case 'f':
                        current_mode = mode_Fill;
                        current_state = state_Finalize;
                    break;

                    case 'm':
                        setcursor((++cursor_sz & 1)? &cursor_sm: &cursor_lg);
                    break;

                    case 'q':
                        DrawingApp.quit = true;
                    break;

                    case '1':
                        bushSize = 1;
                    break;

                    case '2':
                        bushSize = 2;
                    break;

                    case '3':
                        bushSize = 3;
                    break;

                    case '4':
                        bushSize = 4;
                    break;

                    case '5':
                        bushSize = 5;
                    break;

                    case '6':
                        bushSize = 6;
                    break;

                    case '7':
                        bushSize = 7;
                    break;

                    case '8':
                        bushSize = 8;
                    break;

                    case '9':
                        bushSize = 9;
                    break;

                    case '0':
                        bushSize = 10;
                    break;
                }
            break;

            default:
                break;
        }

        // Handle
        I_HandleGUI(&event);
    }

    // Save old mouse pos
    omx = mx;
    omy = my;

    // Get new pos and clamp values
    mx = posx;
    my = posy;

#if UNUSED
    mx = CLAMP(mx, 0, CANVAS_WIDTH + PALETTE_WIDTH);
    my = CLAMP(my, 0, CANVAS_HEIGHT-1);

    omx = CLAMP(omx, 0, CANVAS_WIDTH + PALETTE_WIDTH);
    omy = CLAMP(omy, 0, CANVAS_HEIGHT-1);
#endif
}

// -------------------------------
// Callbacks for GUIButtons
// -------------------------------
void I_HandleGUI(struct event *event)
{
    if(mouseOnPalette == false)
        return;

    // Offset mouse pos
    static int x = 0;
    static int y = 0;

    // For each button
    for(int i = 1; i < PALETTE_BUTTONS_COUNT; i++)
    {
        // If there's a click
        if( event->type == EVT_MOUSEDOWN )
        {
            //If the left mouse button was pressed
            if( event->button == BUTTON_L )
            {
                //Get the mouse offsets
                x = event->x;
                y = event->y;

                //If the mouse is over the button
                if( ( x > paletteButtons[i].box.x ) &&
                    ( x < paletteButtons[i].box.x + paletteButtons[i].box.w ) &&
                    ( y > paletteButtons[i].box.y ) &&
                    ( y < paletteButtons[i].box.y + paletteButtons[i].box.h ) )
                {
                    // Callback
                    paletteButtons[i].OnClick(&paletteButtons[i]);
                }
            }
        }
    }
}
