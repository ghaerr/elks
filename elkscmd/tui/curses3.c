/*
 * curses library workaround - Part III
 *
 * routines mostly used for ttyclock port
 */

#include "curses.h"
#include <stdio.h>

void mvwaddch(WINDOW *w, int y, int x, int ch)
{
    mvaddch(y, x, ch);
}

void mvwaddstr(WINDOW *w, int y, int x, char *str)
{
    move(y, x);
    printw(str);
}

void mvwprintw(WINDOW *w, int y, int x, char *fmt, ...)
{
    va_list ptr;

    move(y, x);
    va_start(ptr, fmt);
    vfprintf(stdout,fmt,ptr);
    va_end(ptr);
}

void mvwin(WINDOW *w, int y, int x)
{
    //yoff = y;
    //xoff = x;
}

void wattron(WINDOW *w, int a)
{
    if (a & (A_BLINK|A_BOLD)) attroff(-1);
    attron(a);
}

void wattroff(WINDOW *w, int a)
{
    if (a & (A_BLINK|A_BOLD)) attroff(-1);
    attroff(a);
}

void wbkgdset(WINDOW *w, int a)
{
    printf("\033[7m");
    attron(a);
}

void wrefresh(WINDOW *w)
{
    printf("\033[m");
}
