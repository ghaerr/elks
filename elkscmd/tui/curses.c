/*
 * curses library workaround
 *  Part I - basic routines
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
    if (isatty(1))
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

void leaveok(void *win, int flag)
{
}

void nodelay(void *win, int flag)
{
}

void scrollok(void *win, int flag)
{
}

void refresh()
{
    fflush(stdout);
}

void mvcur(int oy, int ox, int y, int x)
{
    move(y, x);
}

int addch(int ch)
{
    putchar(ch);
    return OK;
}

int mvaddch(int y, int x, int ch)
{
    printf("\e[%d;%dH%c", y+1, x+1, ch);
    return OK;
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
    printf("\e[%d;%dH", y+1, x+1);
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
