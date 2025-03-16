/*
 * Keyboard and mouse event handling for C86/OWC Graphics
 *
 * 1 Feb 2025 Greg Haerr
 */

#include "event.h"
#include "mouse.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>
#include <termios.h>
#include <errno.h>

int SCREENWIDTH = 640;              /* for mouse clipping, set in graphics_open */
int SCREENHEIGHT = 480;
extern int mouse_fd;
#define SCROLLFACTOR        4       /* multiply factor for scrollwheel */

static struct termios orig;
int posx, posy;                     /* cursor position */

static void open_keyboard(void);
static void close_keyboard(void);

/* wait for event with timeout; msec timeout 0 to poll, timeout -1 to block */
int event_wait_timeout(struct event *e, int timeout)
{
    struct event *event = e;
    int ret;
    fd_set fdset;
    struct timeval timeint, *tv;

    if (timeout == -1)
        tv = NULL;
    else {              /* FIXME just poll, ignore timeout for now */
        timeint.tv_sec = 0;
        timeint.tv_usec = 0;
        tv = &timeint;
    }
    FD_ZERO(&fdset);
    FD_SET(mouse_fd, &fdset);
    FD_SET(0, &fdset);

    for (;;)
    {
        ret = select(mouse_fd + 1, &fdset, NULL, NULL, tv);
        if (ret == 0)
        {
            event->type = EVT_TIMEOUT;
            return 1;
        }
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
quit:
            event->type = EVT_QUIT;
            return 0;
        }
        if (FD_ISSET(0, &fdset))
        {
            unsigned char buf[1];
            if (read(0, buf, sizeof(buf)) > 0)
            {
                if (buf[0] == '\003' || buf[0] == '\001')
                    goto quit;          /* quit on ^C or ^A */
                event->keychar = buf[0];
                event->type = EVT_KEYCHAR;
                return 1;
            }
        }
        if (FD_ISSET(mouse_fd, &fdset))
        {
            int x, y, w, b;
            static int lastx = -1, lasty = -1, lastb = 0;
            if (read_mouse(&x, &y, &w, &b))
            {
                if (b & (BUTTON_SCROLLUP|BUTTON_SCROLLDN))
                {
                    event->type = EVT_MOUSEWHEEL;
                    event->y = w * SCROLLFACTOR;
                    lastb = b;
                    return 1;
                }
                if (b != lastb)
                {
                    if ((b & BUTTON_L) ^ (lastb & BUTTON_L))
                    {
                        event->type = (b & BUTTON_L)? EVT_MOUSEDOWN: EVT_MOUSEUP;
                        event->button = BUTTON_L;
                        event->x = posx;
                        event->y = posy;
                    }
                    else if ((b & BUTTON_R) ^ (lastb & BUTTON_R))
                    {
                        event->type = (b & BUTTON_R)? EVT_MOUSEDOWN: EVT_MOUSEUP;
                        event->button = BUTTON_R;
                        event->x = posx;
                        event->y = posy;
                    }
                    lastb = b;
                    return 1;
                }
                if (x != lastx || y != lasty)
                {
                    event->type = EVT_MOUSEMOVE;
                    posx += x;
                    posy += y;
                    if (posx < 0) posx = 0;
                    if (posy < 0) posy = 0;
                    if (posx >= SCREENWIDTH) posx = SCREENWIDTH - 1;
                    if (posy >= SCREENHEIGHT) posy = SCREENHEIGHT - 1;
                    event->x = posx;
                    event->y = posy;
                    event->xrel = x;
                    event->yrel = y;
                    event->button = b;
                    lastx = x;
                    lasty = y;
                    return 1;
                }
            }
        }
    }
    //event->type = EVT_NONE;
    return 1;
}

/* check for event but don't dequeue if event == NULL */
int event_poll(struct event *event)
{
    static struct event ev;     /* ev.type inited to 0 (=EVT_NONE) */

    if (event == NULL)
    {
        /* only indicate whether event found, don't dequeue */
        if (ev.type == EVT_NONE)
            event_wait_timeout(&ev, 0);     /* 0 timeout = don't block */
        return ev.type;
    }

    /* always deqeue if event found */
    if (ev.type != EVT_NONE)
    {
        *event = ev;
        ev.type = EVT_NONE;
        return event->type;
    }
    event_wait_timeout(event, 0);
    return (event->type == EVT_TIMEOUT)? 0: event->type;
}

int event_open(void)
{
    if (open_mouse() < 0)
        return -1;
    open_keyboard();
    posx = SCREENWIDTH / 2;
    posy = SCREENHEIGHT / 2;


#if DEBUG
    atexit(event_close);
    signal(SIGHUP, catch_signals);
    signal(SIGABRT, catch_signals);
    signal(SIGSEGV, catch_signals);
#endif
    return 0;
}

void event_close(void)
{
    close_mouse();
    close_keyboard();
}

static void open_keyboard(void)
{
    struct termios new;

    tcgetattr(0, &orig);
    new = orig;
    new.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    //new.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT);
    //new.c_cflag &= ~(CSIZE | PARENB);
    new.c_cflag |= CS8;
    new.c_cc[VMIN] = 1;     /* =1 required for lone ESC key processing */
    new.c_cc[VTIME] = 0;
    tcsetattr(0, TCSAFLUSH, &new);
}

static void close_keyboard(void)
{
    tcsetattr(0, TCSANOW, &orig);
}
