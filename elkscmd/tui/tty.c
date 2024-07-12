/* tty management for unikey */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include "unikey.h"

static struct termios oldterm;
static struct termios t;
int _tty_flags;
int iselksconsole;

#define WRITE(FD, SLIT)             write(FD, SLIT, strlen(SLIT))
#define ENABLE_SAFE_PASTE           "\033[?2004h"
#define ENABLE_MOUSE_TRACKING       "\033[?1000;1002;1015;1006h"
#define ENABLE_ALL_MOUSE_TRACKING   "\033[?1000;1003;1015;1006h"
#define DISABLE_MOUSE_TRACKING      "\033[?1000;1002;1003;1015;1006l"
#define RESET_VIDEO                 "\033[1;0;0m\033[?25h\n"
#define PROBE_DISPLAY_SIZE          "\0337\033[9979;9979H\033[6n\0338"
#define GOTO_LASTLINE               "\033[26;0H\033[0J"

static void onkilled(int sig)
{
    exit(1);                /* calls tty_restore in exit handler */
}

void tty_restore(void)
{
    WRITE(1, DISABLE_MOUSE_TRACKING);
    WRITE(1, RESET_VIDEO);
    if (_tty_flags & ExitLastLine)
        WRITE(1, GOTO_LASTLINE);
    tcsetattr(1, TCSADRAIN, &oldterm);
}

void tty_enable_unikey(void)
{
    if (_tty_flags & NoWait) {
        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 0;
    } else {
        t.c_cc[VMIN] = 1;         /* requires ESC ESC ESC for ESC */
        t.c_cc[VTIME] = 1;
    }
    t.c_iflag &= ~(INPCK | ISTRIP | PARMRK | INLCR | IGNCR | ICRNL | IXON |
                   IGNBRK | BRKINT);
    if (_tty_flags & Utf8)
        t.c_iflag |= IUTF8;         /* correct kernel backspace behaviour */
    t.c_lflag &= ~(IEXTEN | ICANON | ECHO | ECHONL | ISIG);
    if (_tty_flags & CatchISig)
        t.c_lflag |= ISIG;
    t.c_cflag &= ~(CSIZE | PARENB);
    t.c_cflag |= CS8;
    tcsetattr(1, TCSADRAIN, &t);
    WRITE(1, ENABLE_SAFE_PASTE);
    if (_tty_flags & FullMouseTracking) {
        WRITE(1, ENABLE_ALL_MOUSE_TRACKING);
    } else if (_tty_flags & MouseTracking) {
        WRITE(1, ENABLE_MOUSE_TRACKING);
    }
}

void tty_fullbuffer(void)
{
    fflush(stdout);
#if ELKS
    /* use existing stdout buffer to save heap space */
    setvbuf(stdout, (char *)stdout->bufstart, _IOFBF, stdout->bufend - stdout->bufstart);
#else
    setvbuf(stdout, NULL, _IOFBF, 0);
#endif
}

void tty_linebuffer(void)
{
    fflush(stdout);
#if ELKS
    /* use existing stdout buffer to save heap space */
    setvbuf(stdout, (char *)stdout->bufstart, _IOLBF, stdout->bufend - stdout->bufstart);
#else
    setvbuf(stdout, NULL, _IOLBF, 0);
#endif
}

int tty_iselksconsole(int fd)
{
#if ELKS
    char *p = ttyname(fd);

    if (!p) return 0;
    return !strncmp(p, "/dev/tty", 8) && p[8] >= '1' && p[8] <= '3';
#else
    return 0;
#endif
}

int tty_init(enum ttyflags f)
{
    static int once;

    _tty_flags = f;
    if (!once) {
        if (tcgetattr(1, &oldterm) != -1) {
            atexit(tty_restore);
            signal(SIGTERM, onkilled);
            signal(SIGINT, onkilled);
        } else {
            return -1;
        }
        once = 1;
        memcpy(&t, &oldterm, sizeof(t));
        iselksconsole = tty_iselksconsole(1);
    }
    tty_enable_unikey();
    if (_tty_flags & FullBuffer)
        tty_fullbuffer();
    return 0;
}
