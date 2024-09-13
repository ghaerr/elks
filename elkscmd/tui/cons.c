/*
 * ELKS Remote Console
 *
 * July 2022 Greg Haerr
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "runes.h"

#define FP_OFF(fp)      ((unsigned)(unsigned long)(void __far *)(fp))
#define FP_SEG(fp)      ((unsigned)((unsigned long)(void __far *)(fp) >> 16))
#define MK_FP(seg,off)  ((void __far *)((((unsigned long)(seg)) << 16) | (off)))

#define poke(s,o,w)     (*((unsigned short __far*)MK_FP((s),(o))) = (w))
#define peek(s,o)       (*((unsigned short __far*)MK_FP((s),(o))))

extern unsigned short kCp437[256];

/* Default 16-color fg, 8 color bg palette                         */
/*                                  0   1   2   3   4   5   6   7
                                   blk blu grn cyn red mag yel wht */
static const int ansi_colors[16] = {30, 34, 32, 36, 31, 35, 33, 37,
                                    90, 94, 92, 96, 91, 95, 93, 97 };

static char *attr_to_ansi(char *buf, unsigned int attr)
{
    sprintf(buf, "\033[%d;%dm",
        ansi_colors[attr & 0x0F], ansi_colors[(attr & 0x70) >> 4] + 10);
    return buf;
}

#define COLS    80
#define LINES   25

static void display(void)
{
    int r, c, a;
    int b, ch;
    unsigned short __far *chattr = MK_FP(0xb800, 0);
    char buf[16];

    fputs("\033[H", stdout);
    for (r=0; r<LINES; r++) {
        a = -1;
        for (c=0; c<COLS; c++) {
            b = chattr[r*COLS + c];
            ch = kCp437[b & 255];
            if (a != (b & 0xFF00)) {
                fputs(attr_to_ansi(buf, b >> 8), stdout);
                a = b & 0xFF00;
            }
            if (runetostr(buf, ch)) {
                fputs(buf, stdout);
            }
        }
        putc(r == LINES - 1 ? '\r' : '\n', stdout);
    }
    fputs("\033[1;0;0m", stdout);
    fflush(stdout);
}

int main(int ac, char **av)
{
    for (;;) {
        display();
        sleep(1);
    }
}
