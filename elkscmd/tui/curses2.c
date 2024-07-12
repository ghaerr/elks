/*
 * curses library workaround, Part II
 *
 * Jul 2022 Greg Haerr
 */
#include "curses.h"
#include "unikey.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <termios.h>

static int mouse_button;

void waitformouse(void)
{
    int n, e;
    int mx, my, modkeys;
    char buf[32];

    if (mouse_button != kMouseLeftDown && mouse_button != kMouseLeftDoubleClick)
        return;
    for (;;) {
        if ((n = readansi(0, buf, sizeof(buf))) < 0)
            break;
        if ((n = ansi_to_unimouse(buf, n, &mx, &my, &modkeys, &e)) != -1) {
            if (n == kMouseLeftUp)
                return;
        }
        /* ignore keystrokes */
    }
}

int getch()
{
    int e, n;
    int mx, my, modkeys, status;
    char buf[32];

    fflush(stdout);
    if ((n = readansi(0, buf, sizeof(buf))) < 0)
        return -1;
    if ((e = ansi_to_unikey(buf, n)) != -1) {   // FIXME UTF-8 unicode != -1
        //printf("KBD %x (%d)\n", e, n);
        return e;
    }
    if ((n = ansi_to_unimouse(buf, n, &mx, &my, &modkeys, &status)) != -1) {
        mouse_button = n;
        switch (n) {
        case kMouseLeftDoubleClick:
            waitformouse();
            return n;
        case kMouseWheelDown:
        case kMouseWheelUp:
            return n;
        }
    }
    return -1;
}

void wgetnstr(void *win, char *str, int n)
{
    tty_restore();
    if (fgets(str, n, stdin))
        str[strlen(str)-1] = '\0';
    else str[0] = '\0';
    tty_enable_unikey();
}

void start_color()
{
}

int use_default_colors()
{
    return OK;
}

struct cp {
    int fg;
    int bg;
};

static struct cp attrs[17] = {
    { -1, -1 },     /* entry 0 is default attribute */
};

void init_pair(int ndx, int fg, int bg)
{
    if (ndx >= 1 && ndx <= 16) {
        attrs[ndx].fg = fg;
        attrs[ndx].bg = bg;
    }
}

/*                                  0   1   2   3   4   5   6   7
                                   blk blu grn cyn red mag yel wht */
static const int ansi_colors[16] = {30, 34, 32, 36, 31, 35, 33, 37,
                                    90, 94, 92, 96, 91, 95, 93, 97 };

void attron(int a)
{
    int fg = attrs[a & 0x0F].fg;
    int bg = attrs[a & 0x0F].bg;

    if (fg == -1) fg = 39; else fg = ansi_colors[fg];
    if (bg == -1) bg = 39; else bg = ansi_colors[bg];
    if (bg == 39)
        printf("\033[%dm", fg);
    else printf("\033[%d;%dm", fg, bg+10);
}

void attroff(int a)
{
    printf("\033[1;0;0m");
}
