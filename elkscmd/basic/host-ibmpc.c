/*
 * Architecture Specific routines for CGA
 * Sep 2024 Takahiro Yamada
 */
#include <stdio.h>
#include <stdlib.h>
#include <arch/io.h>
#include "host.h"
#include "basic.h"

/* supported graphics modes in host_mode() */
#define VGA_640x350x16      0x10        /* 640x350 16 color/4bpp */
#define VGA_640x480x16      0x12        /* 640x480 16 color/4bpp */
#define TEXT_MODE           0x03        /* 80x25 text mode */

static int gmode;
static int MAX_Y;
static char exit_on = 0;

typedef struct {
    int x;
    int y;
    int fgc;
    int bgc;
    int r;
} xyc_t;

static xyc_t gxyc = {0, 0, 7, 0, 1};

void fmemsetw(void * off, unsigned int seg, unsigned int val, size_t count);
void int_10(unsigned int ax, unsigned int bx, unsigned int cx, unsigned int dx);

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
        int_10(TEXT_MODE, 0, 0, 0);
}

void host_mode(int mode) {
    gmode = mode;

    if (gmode && !exit_on) {
        atexit(mode_reset);
        exit_on = 1;
    }

    if (gmode) {
        if (getenv("EGAMODE") != NULL) {
            gmode = VGA_640x350x16;
            MAX_Y = 350 - 1;
        } else {
            gmode = VGA_640x480x16;
            MAX_Y = 480 - 1;
        }
        int_10(gmode, 0, 0, 0);
    } else
        int_10(TEXT_MODE, 0, 0, 0);
}

void host_cls() {

    if (gmode)
        fmemsetw(0, 0xA000, 0, 19200);  /* 640*480/8 bytes VGA ram */
    else
        fprintf(outfile, "\033[H\033[2J");
}

void host_color(int fgc, int bgc) {

    if (gmode) {
        gxyc.fgc = fgc;
        gxyc.bgc = bgc;
    }
}

static void draw_point(int x, int y)
{
    int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, x, y);
}

void host_plot(int x, int y) {

    if (gmode) {
        y = MAX_Y - y;

        draw_point(x, y);

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
        y = MAX_Y - y;

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
                            draw_point(nx, ny);
                        }
                        nydiff &= 0x007F;
                    }
                    else if (ny > y) {
                        for (ni = 0; ni < (nydiff >> 7); ni++) {
                            ny--;
                            draw_point(nx, ny);
                        }
                        nydiff &= 0x007F;
                    }
                    else
                        draw_point(nx, ny);
                }
            }
            else if (nx > x) {
                while (nx > x) {
                    nx--;
                    nydiff += ydiff;
                    if (ny < y) {
                        for (ni = 0; ni < (nydiff >> 7); ni++) {
                            ny++;
                            draw_point(nx, ny);
                        }
                        nydiff &= 0x007F;
                    }
                    else if (ny > y) {
                        for (ni = 0; ni < (nydiff >> 7); ni++) {
                            ny--;
                            draw_point(nx, ny);
                        }
                        nydiff &= 0x007F;
                    }
                    else
                        draw_point(nx, ny);
                }
            }
            else if (nx == x) {
                if (ny < y) {
                    while (ny < y) {
                        ny++;
                        draw_point(nx, ny);
                    }
                }
                else if (ny > y) {
                    while (ny > y) {
                        ny--;
                        draw_point(nx, ny);
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
                            draw_point(nx, ny);
                        }
                        nxdiff &= 0x007F;
                    }
                    else if (nx > x) {
                        for (ni = 0; ni < (nxdiff >> 7); ni++) {
                            nx--;
                            draw_point(nx, ny);
                        }
                        nxdiff &= 0x007F;
                    }
                    else
                        draw_point(nx, ny);
                }
            }
            else if (ny > y) {
                while (ny > y) {
                    ny--;
                    nxdiff += xdiff;
                    if (nx < x) {
                        for (ni = 0; ni < (nxdiff >> 7); ni++) {
                            nx++;
                            draw_point(nx, ny);
                        }
                        nxdiff &= 0x007F;
                    }
                    else if (nx > x) {
                        for (ni = 0; ni < (nxdiff >> 7); ni++) {
                            nx--;
                            draw_point(nx, ny);
                        }
                        nxdiff &= 0x007F;
                    }
                    else
                        draw_point(nx, ny);
                }
            }
            else if (ny == y) {
                if (nx < x) {
                    while (nx < x) {
                        nx++;
                        draw_point(nx, ny);
                    }
                }
                else if (nx > x) {
                    while (nx > x) {
                        nx--;
                        draw_point(nx, ny);
                    }
                }
            }
        }
        draw_point(nx, ny);

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
