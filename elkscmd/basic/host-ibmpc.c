/*
 * Architecture Specific routines for CGA
 * Sep 2024 Takahiro Yamada
 */
#include <stdio.h>
#include <stdlib.h>
#include <arch/io.h>
#include "host.h"
#include "basic.h"

#define VIDEO_05_G320x200   0x0005
#define VIDEO_03_T80x25     0x0003

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

extern void fmemsetw(void * off, unsigned int seg, unsigned int val, size_t count);

extern void int_10(unsigned int ax, unsigned int bx,
                   unsigned int cx, unsigned int dx);

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

static void mode_reset() {
    if (gmode)
        int_10(VIDEO_03_T80x25, 0, 0, 0);
}

void host_mode(int mode) {
    gmode = mode;

    if (gmode && !exit_on) {
        atexit(mode_reset);
        exit_on = 1;
    }

    if (gmode)
        int_10(VIDEO_05_G320x200, 0, 0, 0);
    else
        int_10(VIDEO_03_T80x25, 0, 0, 0);
}

void host_cls() {

    if (gmode) {
        fmemsetw(0, 0xB800, 0, 4000);
        fmemsetw(0, 0xBA00, 0, 4000);
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

//using midpoint circle algorithm
void host_circle(int xc, int yc, int r) {
    int x = 0;
    int y = r;
    int d = 1 - r;

    while (x <= y) {
        host_plot(xc + x, yc + y);
        host_plot(xc - x, yc + y);
        host_plot(xc + x, yc - y);
        host_plot(xc - x, yc - y);
        host_plot(xc + y, yc + x);
        host_plot(xc - y, yc + x);
        host_plot(xc + y, yc - x);
        host_plot(xc - y, yc - x);

        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
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
