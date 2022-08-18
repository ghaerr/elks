/* IBM PC cp437 display routines */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <time.h>
#include "unikey.h"
#include "runes.h"

/* 16-color palette positions (not ANSI colors) */
#define BLACK         0
#define BLUE          1
#define GREEN         2
#define CYAN          3
#define RED           4
#define MAGENTA       5
#define BROWN         6
#define LIGHTGRAY     7
#define DARKGRAY      8
#define LIGHTBLUE     9
#define LIGHTGREEN   10
#define LIGHTCYAN    11
#define LIGHTRED     12
#define LIGHTMAGENTA 13
#define YELLOW       14
#define WHITE        15

/* Default 16-color fg, 8 color bg palette                         */
/*                                  0   1   2   3   4   5   6   7
                                   blk blu grn cyn red mag yel wht */
static const int ansi_colors[16] = {30, 34, 32, 36, 31, 35, 33, 37,
                                    90, 94, 92, 96, 91, 95, 93, 97 };

/* two 16-color palettes, fg_pal256 used on 256-color capable terminals */
static const int *fg_pal16 = ansi_colors;
static const int *fg_pal256;

/* Set non-standard 16-color fg palettes, for use on 16-color or 256-color terminals */
void tty_setfgpalette(const int *pal16, const int *pal256)
{
    fg_pal16 = pal16;
    fg_pal256 = pal256;
}

static char *attr_to_ansi(char *buf, unsigned int attr)
{
    int fg = attr & 0x0F;               /* 16 fg colors */
    int bg = (attr & 0x70) >> 4;        /*  8 bg colors */

    if (fg_pal256 && !iselksconsole) {
        sprintf(buf, "\e[38;5;%dm\e[%dm", fg_pal256[fg], ansi_colors[bg] + 10);
    } else {
        sprintf(buf, "\e[%d;%dm", fg_pal16[fg], ansi_colors[bg] + 10);
    }
    return buf;
}

#if ELKS
static int elks_displayable(int c)
{
    switch (c) {
    case '\0':
    case '\007':
    case '\b':
    case '\t':
    case '\r':
    case '\n':
    case '\033':
        return 0;
    }
    return 1;
}
#endif

/* convert CP 437 byte to string + NUL, depending on platform */
int cp437tostr(char *s, int c)
{
    extern unsigned short kCp437[256];

#if ELKS
    if (iselksconsole) {
        if (!elks_displayable(c & 255))
            c = '?';
        s[0] = (char)c;
        s[1] = '\0';
        return 1;
    }
#endif
    return runetostr(s, kCp437[c & 255]);
}

static int COLS = 80;
static int LINES = 25;
static char *video_ram;

char *tty_allocate_screen(int cols, int lines)
{
    if (cols) COLS = cols;
    if (lines) LINES = lines;
    video_ram = calloc(COLS * LINES * 2, 1);
    return video_ram;
}

void tty_output_screen(int flush)
{
    int r, c, a, b;
    unsigned short *chattr = (unsigned short *)video_ram;
    char buf[16];

    printf("\e[?25l\e[H");      /* cursor off, home */
    for (r=0; r<LINES; r++) {
        a = -1;
        for (c=0; c<COLS; c++) {
            b = *chattr++;
            if (a != (b & 0xFF00)) {
                fputs(attr_to_ansi(buf, b >> 8), stdout);
                a = b & 0xFF00;
            }
            if (cp437tostr(buf, b & 255))
                fputs(buf, stdout);
        }
        if (r == LINES - 1)
            printf("\r");
        else printf("\n");
    }
    printf("\e[1;0;0m");           /* reset attrs, cursor left off */
    if (flush)
        fflush(stdout);
}
