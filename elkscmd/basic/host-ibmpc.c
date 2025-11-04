/*
 * Architecture Specific routines for IBM PC BASIC with EGA/VGA graphics support
 * Sep 2024 Takahiro Yamada
 */
#include <stdio.h>
#include <stdlib.h>
#include <arch/io.h>
#include "host.h"
#include "basic.h"

/* supported graphics modes */
#define VGA_640x350x16      0x10        /* 640x350 16 color/4bpp */
#define VGA_640x480x16      0x12        /* 640x480 16 color/4bpp */
#define TEXT_MODE           0x03        /* 80x25 text mode */

/* graphics context */
struct gc {
    int x;
    int y;
    int fg;
    int bg;
    int r;
};

static int gmode;
static int MAX_Y;
static char exit_on = 0;
static struct gc gc = {0, 0, 7, 0, 1};

/* external procedures */
void fmemsetw(void * off, unsigned int seg, unsigned int val, size_t count);
void int_10(unsigned int ax, unsigned int bx, unsigned int cx, unsigned int dx);

void host_digitalWrite(int pin,int state)
{
}

int host_digitalRead(int pin)
{
    return 0;
}

int host_analogRead(int pin)
{
    return 0;
}

void host_pinMode(int pin,int mode)
{
}

static void mode_reset(void)
{
    if (gmode)
        int_10(TEXT_MODE, 0, 0, 0);
}

void host_mode(int mode)
{
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

void host_cls(void)
{
    if (gmode)
        fmemsetw(0, 0xA000, 0, 19200);  /* 640*480/8 bytes VGA ram */
    else
        fprintf(outfile, "\033[H\033[2J");
}

void host_color(int fg, int bg)
{
    if (gmode) {
        gc.fg = fg;
        gc.bg = bg;
    }
}

static void draw_point(int x, int y)
{
    int_10((0x0C00 | (0xFF & gc.fg)), 0, x, y);
}

static void draw_line(int x1, int y1, int x2, int y2)
{
    /* Bresenham's line algorithm for efficient line drawing */
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (x1 != x2 || y1 != y2) {
        draw_point(x1, y1);

        int e2 = err << 1;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
    draw_point(x2, y2);
}

void host_plot(int x, int y)
{
    if (gmode) {
        y = MAX_Y - y;

        draw_point(x, y);

        gc.x = x;
        gc.y = y;
    }
}

void host_draw(int x, int y)
{
    if (gmode) {
        y = MAX_Y - y;
        draw_line(gc.x, gc.y, x, y);
        gc.x = x;
        gc.y = y;
    }
}

//using midpoint circle algorithm
void host_circle(int xc, int yc, int r)
{
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

void host_outb(int port, int value)
{
    outb(value, port);
}

void host_outw(int port, int value)
{
    outw(value, port);
}

int host_inpb(int port)
{
    return inb(port);
}

int host_inpw(int port)
{
    return inw(port);
}
