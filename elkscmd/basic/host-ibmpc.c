/*
 * Architecture Specific routines for CGA
 * Sep 2024 Takahiro Yamada
 */
#include <stdio.h>
#include <stdlib.h>
#if __ia16__
#include <arch/io.h>
#else
#define inb(port)           0
#define inw(port)           0
#define outb(value,port)
#define outw(value,port)
#endif
#include "host.h"
#include "basic.h"

#define VIDEO_05_G320x200   0x0005
#define VIDEO_03_T80x25     0x0003

extern FILE *outfile;

static int gmode = 0;
static int exit_on = 0;

typedef struct {
    int x;
    int y;
    int fgc;
    int bgc;
    int r;
} xyc_t;

static xyc_t gxyc = {0, 0, 7, 0, 1};

#if __ia16__
void int_10(unsigned int ax, unsigned int bx,
            unsigned int cx, unsigned int dx)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("int $0x10;"
                      :
                      :"a" (ax), "b" (bx), "c" (cx), "d" (dx)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void memclrw(unsigned int offset, seg_t seg, unsigned int count)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %%ax,%%di;"
                      "mov %%bx,%%es;"
                      "xor %%ax,%%ax;"
                      "cld;"
                      "rep;"
                      "stosw;"
                      :
                      :"a" (offset), "b" (seg), "c" (count)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}
#else
void int_10(unsigned int ax, unsigned int bx,
            unsigned int cx, unsigned int dx)
{
}

void memclrw(unsigned int offset, seg_t seg, unsigned int count)
{
}
#endif

void host_digitalWrite(int pin,int state) {
}

int host_digitalRead(int pin) {
    return 0;
}

int host_analogRead(int pin) {
    return 0;
}

void host_pinMode(int pin,int mode) {
}

void host_mode(int mode) {
    gmode = mode;

    if (gmode && !exit_on) {
        atexit(host_exit);
        exit_on = 1;
    }

    if (gmode)
        int_10(VIDEO_05_G320x200, 0, 0, 0);
    else
        int_10(VIDEO_03_T80x25, 0, 0, 0);
}

void host_cls() {

    if (gmode) {
        memclrw(0, 0xB800, 4000);
        memclrw(0, 0xBA00, 4000);
    }
    else
        fprintf(outfile, "\033[H\033[2J");
}

void host_color(int fgc, int bgc) {

    if (gmode) {
        gxyc.fgc = fgc;
        gxyc.bgc = bgc;
    }
}

void host_plot(int x, int y) {

    if (gmode) {
        y = 199 - y;

        int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, x, y);

        gxyc.x = x;
        gxyc.y = y;
    }
}

void host_draw(int x, int y) {

    int nx;
    int ny;
    unsigned int fdiff;
    unsigned int xdiff;
    unsigned int ydiff;
    unsigned int nxdiff;
    unsigned int nydiff;
    int ni;

    nx = gxyc.x;
    ny = gxyc.y;

    if (gmode) {
        y = 199 - y;

        xdiff = 0;
        ydiff = 0;
        nxdiff = 0;
        nydiff = 0;

        if ((nx < x) && (ny < y)) {
            if ((y - ny) > (x - nx)) {
                fdiff = (y - ny) << 7;                         // binary fraction 7bits
                ydiff = (unsigned int) (fdiff / (x - nx));    // maximum slope is 511.992...
            }
            else {
                fdiff = (x - nx) << 7;
                xdiff = (unsigned int) (fdiff / (y - ny));
            }
        }
        else if ((nx > x) && (ny < y)) {
            if ((y - ny) > (nx - x)) {
                fdiff = (y - ny) << 7;
                ydiff = (unsigned int) (fdiff / (nx - x));
            }
            else {
                fdiff = (nx - x) << 7;
                xdiff = (unsigned int) (fdiff /(y - ny));
            }
        }
        else if ((nx < x) && (ny > y)) {
            if ((ny - y) > (x - nx)) {
                fdiff = (ny - y) << 7;
                ydiff = (unsigned int) (fdiff /(x - nx));
            }
            else {
                fdiff = (x - nx) << 7;
                xdiff = (unsigned int) (fdiff /(ny - y));
            }
        }
        else if ((nx > x) && (ny > y)) {
            if ((ny - y) > (nx - x)) {
                fdiff = (ny - y) << 7;
                ydiff = (unsigned int) (fdiff /(nx - x));
            }
            else {
                fdiff = (nx - x) << 7;
                xdiff = (unsigned int) (fdiff /(ny - y));
            }
        }

        if (xdiff == 0) {
            if (nx < x) {
                while (nx < x) {
                    nx++;
                    nydiff += ydiff;
                    if (ny < y) {
                        for (ni = 0; ni < (nydiff >> 7); ni++) {
                            ny++;
                            int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                        }
                        nydiff &= 0x007F;
                    }
                    else if (ny > y) {
                        for (ni = 0; ni < (nydiff >> 7); ni++) {
                            ny--;
                            int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                        }
                        nydiff &= 0x007F;
                    }
                    else
                        int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                }
            }
            else if (nx > x) {
                while (nx > x) {
                    nx--;
                    nydiff += ydiff;
                    if (ny < y) {
                        for (ni = 0; ni < (nydiff >> 7); ni++) {
                            ny++;
                            int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                        }
                        nydiff &= 0x007F;
                    }
                    else if (ny > y) {
                        for (ni = 0; ni < (nydiff >> 7); ni++) {
                            ny--;
                            int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                        }
                        nydiff &= 0x007F;
                    }
                    else
                        int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                }
            }
            else if (nx == x) {
                if (ny < y) {
                    while (ny < y) {
                        ny++;
                        int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                    }
                }
                else if (ny > y) {
                    while (ny > y) {
                        ny--;
                        int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                    }
                }
            }
        }
        else {
            if (ny < y) {
                while (ny < y) {
                    ny++;
                    nxdiff += xdiff;
                    if (nx < x) {
                        for (ni = 0; ni < (nxdiff >> 7); ni++) {
                            nx++;
                            int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                        }
                        nxdiff &= 0x007F;
                    }
                    else if (nx > x) {
                        for (ni = 0; ni < (nxdiff >> 7); ni++) {
                            nx--;
                            int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                        }
                        nxdiff &= 0x007F;
                    }
                    else
                        int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                }
            }
            else if (ny > y) {
                while (ny > y) {
                    ny--;
                    nxdiff += xdiff;
                    if (nx < x) {
                        for (ni = 0; ni < (nxdiff >> 7); ni++) {
                            nx++;
                            int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                        }
                        nxdiff &= 0x007F;
                    }
                    else if (nx > x) {
                        for (ni = 0; ni < (nxdiff >> 7); ni++) {
                            nx--;
                            int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                        }
                        nxdiff &= 0x007F;
                    }
                    else
                        int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                }
            }
            else if (ny == y) {
                if (nx < x) {
                    while (nx < x) {
                        nx++;
                        int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                    }
                }
                else if (nx > x) {
                    while (nx > x) {
                        nx--;
                        int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, nx, ny);
                    }
                }
            }
        }
        int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, x, y);

        gxyc.x = x;
        gxyc.y = y;
    }
}

void host_circle(int x, int y, int r) {
}

void host_outb(int port, int value) {
    outb(value, port);
}

void host_outw(int port, int value) {
    outw(value, port);
}

int host_inpb(int port) {
    return inb(port);
}

int host_inpw(int port) {
    return inw(port);
}

void host_exit() {
    if (gmode)
        int_10(VIDEO_03_T80x25, 0, 0, 0);
}
