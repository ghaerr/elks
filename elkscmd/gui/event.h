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
    int     button;         /* mouse BUTTON_xxx on EVT_MOUSE* events */
    int     x;              /* mouse or wheel location on EVT_MOUSE* ior EVT_WHEEL* */
    int     y;
    int     xrel;           /* relative location on EVT_WHEEL* events */
    int     yrel;
};

/* event handling */
int event_open(void);
void event_close(void);

/* wait/poll on event, timeout in msecs or = -1 blocking */
int event_wait_timeout(struct event *e, int timeout);   /* returns 0 on EVT_QUIT */
int event_poll(struct event *event);                    /* polls if event == NULL */

extern int posx, posy;      /* cursor position */

#endif
