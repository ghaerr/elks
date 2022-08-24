/*
 * curses library workaround
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

int LINES = 25;
int COLS = 80;
void *stdscr;

void *initscr()
{
    tty_init(MouseTracking|CatchISig|FullBuffer);
    tty_getsize(&COLS, &LINES);
    return stdout;
}

void endwin()
{
    tty_restore();
    tty_linebuffer();
}

int has_colors()
{
    return 1;
}

void cbreak()
{
}

void noecho()
{
}

void nonl()
{
}

void intrflush(void *win, int bf)
{
}

void keypad(void *win, int bf)
{
}

void echo()
{
}

void timeout(int t)
{
}

/* cursor on/off */
void curs_set(int visibility)
{
    printf("\e[?25%c", visibility? 'h': 'l');
}

/* clear screen */
void erase()
{
    printf("\e[H\e[2J");
}

void move(int y, int x)
{
    printf("\e[%d;%dH", y, x);
}

void clrnl(void)
{
    printf("\e[0K\n");
}

void clrtoeos(void)
{
    printf("\e[0J");
}

void clrtoeol(void)
{
    printf("\e[0K");
}

void printw(char *fmt, ...)
{
    va_list ptr;

    va_start(ptr, fmt);
    vfprintf(stdout,fmt,ptr);
    va_end(ptr);
}

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

void use_default_colors()
{
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

/*                                           blk blu grn cyn red mag yel wht */
static const unsigned char ansi_colors[8] = {30, 34, 32, 36, 31, 35, 33, 37 };

void attron(int a)
{
    int fg = attrs[a & 0x0F].fg;
    int bg = attrs[a & 0x0F].bg;

    if (fg == -1) fg = 39; else fg = ansi_colors[fg];
    if (bg == -1) bg = 39; else bg = ansi_colors[bg];
    if (bg == 39)
        printf("\e[%dm", fg);
    else printf("\e[%d;%dm", fg, bg+10);
}

void attroff(int a)
{
    printf("\e[1;0;0m");
}
