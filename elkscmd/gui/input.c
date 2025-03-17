#include <stdlib.h>
#include "input.h"
#include "app.h"
#include "render.h"
#include "gui.h"
#include "graphics.h"
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
                if(event.button == BUTTON_L)
                    drawing = true;
                else
                    altdrawing = true;
                hidecursor();

                // Get latest mouse
                omx = mx;
                omy = my;

                mx = event.x;
                my = event.y;
            break;

            case EVT_MOUSEUP:
                if(event.button == BUTTON_L)
                    drawing = false;
                else
                    altdrawing = false;
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
                        floodFillCalled = true;
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
    mx = CLAMP(mx, 0, SCREEN_WIDTH + PALETTE_WIDTH);
    my = CLAMP(my, 0, SCREEN_HEIGHT-1);

    omx = CLAMP(omx, 0, SCREEN_WIDTH + PALETTE_WIDTH);
    omy = CLAMP(omy, 0, SCREEN_HEIGHT-1);
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
