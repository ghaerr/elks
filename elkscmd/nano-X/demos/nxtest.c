#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include "nano-X.h"
/*
 * Nano-X simple drawing program for testing
 */

static GR_WINDOW_ID win;
static GR_GC_ID gc;
static GR_SCREEN_INFO si;
static int x, y;
static int w = 100;
static int h = 100;
static int dir = 5;

#if 0
#include <sys/time.h>
#include <sys/select.h>
/*
 * Suspend execution of the program for the specified number of milliseconds.
 */
void
GdDelay(unsigned long msecs)
{
    struct timeval timeval;

    timeval.tv_sec = msecs / 1000;
    timeval.tv_usec = (msecs % 1000) * 1000;
    select(0, NULL, NULL, NULL, &timeval);
}
#endif

int main(int argc, char ** argv)
{
    if (GrOpen() < 0) {
        GrClose();
        printf("Can't open graphics\n");
        exit(1);
    }
    
    GrGetScreenInfo(&si);

    win = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, w, h,
        0, WHITE, BLACK);

    GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_KEY_DOWN);
    GrSelectEvents(win, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_KEY_DOWN
                       | GR_EVENT_MASK_EXPOSURE);

    gc = GrNewGC();
    GrSetGCForeground(gc, BLACK);
    GrFillRect(GR_ROOT_WINDOW_ID, gc, 0, 0, si.cols, si.rows);

    GrMapWindow(win);

    GR_TIMEOUT timeout = 1;
    while (1) {
        GR_EVENT event;
        GR_EVENT_KEYSTROKE *ek = &event.keystroke;

        GrGetNextEventTimeout(&event, timeout);
        switch (event.type) {
        case GR_EVENT_TYPE_KEY_DOWN:
            switch (ek->ch) {
            case '-':   /* slow down */
                timeout += 1;
                continue;
            case '+':   /* speed up */
            case '=':
                timeout -= 1;
                 if ((long)timeout < -1) timeout = GR_TIMEOUT_POLL;
                 continue;
            case 'q':
                GrClose();
                exit(0);
            }
            break;
        case GR_EVENT_TYPE_EXPOSURE:
            /* don't generate continuous exposure events from GrMoveWindow */
            continue;
        case GR_EVENT_TYPE_NONE:
            break;
        }

        if (x + dir + w >= si.cols || y + dir + h >= si.rows ||
            x + dir < 0 || y + dir  < 0) {
                dir = -dir;
        }
        x += dir;
        y += dir;
        GrMoveWindow(win, x, y);
    }
}
