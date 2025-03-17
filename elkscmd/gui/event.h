#ifndef EVENT_H
#define EVENT_H

/* event.h */

/* event types */
#define EVT_NONE            0
#define EVT_TIMEOUT         1
#define EVT_QUIT            2
#define EVT_KEYCHAR         3
#define EVT_MOUSEDOWN       4
#define EVT_MOUSEUP         5
#define EVT_MOUSEMOVE       6
#define EVT_MOUSEWHEEL      7

/* mouse button values */
#define BUTTON_L            0x01      /* left button*/
#define BUTTON_R            0x02      /* right button*/
#define BUTTON_M            0x10      /* middle*/
#define BUTTON_SCROLLUP     0x20      /* wheel up*/
#define BUTTON_SCROLLDN     0x40      /* wheel down*/

struct event {
    int     type;           /* event type */
    int     keychar;        /* keyboard character on EVT_KEYBOARD */
    int     button;         /* mouse BUTTON_L/BUTTON_R on EVT_MOUSE* events */
    int     x;              /* mouse position on all events */
    int     y;
    int     xrel;           /* relative location on EVT_MOUSEMOVE events */
    int     yrel;
    int     w;              /* wheel movement on EVT_WHEEL* events */
};

/* event handling */
int event_open(void);
void event_close(void);

/* wait/poll on event, timeout in msecs or EV_BLOCK/EV_POLL */
#define EV_BLOCK    (-1)
#define EV_POLL     0

int event_wait_timeout(struct event *e, int timeout);   /* returns 0 on EVT_QUIT */
int event_poll(struct event *event);                    /* polls if event == NULL */

extern int posx, posy;      /* cursor position */

#endif
