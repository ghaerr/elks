/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * v7show - put an image on the screen of a Video Seven VEGA VGA
 *
 * usage: v7show [-t secs] [-v] [-k] [-F] file.v7i
 *
 * The card's own 6F05 sets the mode and 6F04 is asked what it actually got.
 * That readback is the point rather than a formality: which high resolution
 * modes a board can reach depends on its memory and on the monitor strapped to
 * it, so a mode this program cannot confirm is one it refuses to draw into
 * instead of leaving a picture nobody can see.
 *
 * The 16 colour modes are planar.  A byte written with the Map Mask selecting
 * one plane lands in that plane alone, so the file holds each plane whole, one
 * after another, and the blit is four passes rather than any bit shuffling
 * here.  Mode 13h is the chunky exception and is copied straight through.
 *
 * -v reads display memory back and compares it against the file.  Without a
 * screen that is the only honest way to say the picture arrived, and it
 * doubles as a memory test of the card: a plane that reads back wrong is a
 * fault whether or not anyone is looking at the monitor.
 *
 * Copyright (C) 2026 G Keet
 * Licensed under the GNU General Public License version 2, the same terms as
 * the ELKS kernel.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "v7.h"

#define VRAM_LIN        0xA0000000UL    /* A000:0000 as a far pointer */
#define SEQ_MAPMASK     0x02
#define GC_INDEX        0x3CE
#define GC_READMAP      0x04

#define V7I_MAGIC       "V7I1"
#define V7I_HDR         14
#define CHUNK           512

struct v7i {
    unsigned char mode;
    unsigned char planes;
    unsigned int width, height, stride;
};

static unsigned char buf[CHUNK];
static unsigned char vbuf[CHUNK];

static void usage(void)
{
    fprintf(stderr, "usage: v7show [-t secs] [-v] [-k] [-F] file.v7i\n");
    fprintf(stderr, "  -t  hold the picture this long (default 5)\n");
    fprintf(stderr, "  -v  read display memory back and check it\n");
    fprintf(stderr, "  -k  keep the mode, do not restore text\n");
    fprintf(stderr, "  -F  draw a mode the detected monitor cannot scan\n");
    fprintf(stderr, "G Keet, 2026\n");
    exit(1);
}

static int read_header(int fd, struct v7i *h)
{
    unsigned char b[V7I_HDR];

    if (read(fd, (char *)b, V7I_HDR) != V7I_HDR) {
        fprintf(stderr, "v7show: short file\n");
        return -1;
    }
    if (memcmp(b, V7I_MAGIC, 4)) {
        fprintf(stderr, "v7show: not a v7i image\n");
        return -1;
    }
    h->mode   = b[4];
    h->planes = b[5];
    h->width  = b[6]  | ((unsigned int)b[7] << 8);
    h->height = b[8]  | ((unsigned int)b[9] << 8);
    h->stride = b[10] | ((unsigned int)b[11] << 8);
    if (h->planes != 1 && h->planes != 4) {
        fprintf(stderr, "v7show: %u planes is not 1 or 4\n", h->planes);
        return -1;
    }
    return 0;
}

/*
 * Set the mode and make the card say what it gave.  6F04 answers in columns
 * and rows of text even in a graphics mode, so the test is that the mode
 * number came back, not the geometry: a card that cannot do the mode leaves
 * the old one in force and that is what shows up here.
 */
static int set_mode(unsigned char mode)
{
    unsigned int ax, bx, cx;

    ax = V7_BIOS_FUNC | V7_BIOS_SETMODE;
    bx = mode;
    cx = 0;
    v7_bios(&ax, &bx, &cx);

    ax = V7_BIOS_FUNC | V7_BIOS_GETMODE;
    bx = cx = 0;
    v7_bios(&ax, &bx, &cx);
    if ((ax & 0xFF) != mode) {
        fprintf(stderr, "v7show: mode %02x not available, card stayed in %02x\n",
                mode, (unsigned int)(ax & 0xFF));
        return -1;
    }
    return 0;
}

static void select_write_plane(unsigned int pl)
{
    outb_p(SEQ_MAPMASK, V7_SEQ_INDEX);
    outb_p((unsigned char)(1U << pl), V7_SEQ_DATA);
}

static void select_read_plane(unsigned int pl)
{
    outb_p(GC_READMAP, GC_INDEX);
    outb_p((unsigned char)pl, GC_INDEX + 1);
}

/* Leave the Map Mask as a mode set leaves it, all four planes enabled. */
static void restore_planes(void)
{
    outb_p(SEQ_MAPMASK, V7_SEQ_INDEX);
    outb_p(0x0F, V7_SEQ_DATA);
}

static long blit(int fd, struct v7i *h, int verify)
{
    unsigned char __far *v;
    unsigned long plbytes = (unsigned long)h->stride * h->height;
    unsigned long done, n;
    unsigned int pl, i;
    long bad = 0;

    for (pl = 0; pl < h->planes; pl++) {
        if (h->planes > 1)
            select_write_plane(pl);
        v = (unsigned char __far *)VRAM_LIN;
        for (done = 0; done < plbytes; done += n) {
            n = plbytes - done;
            if (n > CHUNK)
                n = CHUNK;
            if (read(fd, (char *)buf, (int)n) != (int)n) {
                fprintf(stderr, "v7show: image ends early in plane %u\n", pl);
                return -1;
            }
            for (i = 0; i < (unsigned int)n; i++)
                v[done + i] = buf[i];
        }
    }
    if (!verify)
        return 0;

    /*
     * Read every plane back.  The file has to be walked a second time, so
     * seek rather than hold a whole plane in memory: 38K a plane at 640x480
     * is more than this program is entitled to.
     */
    for (pl = 0; pl < h->planes; pl++) {
        if (lseek(fd, (off_t)V7I_HDR + (off_t)pl * plbytes, SEEK_SET) < 0) {
            perror("v7show: lseek");
            return -1;
        }
        if (h->planes > 1)
            select_read_plane(pl);
        v = (unsigned char __far *)VRAM_LIN;
        for (done = 0; done < plbytes; done += n) {
            n = plbytes - done;
            if (n > CHUNK)
                n = CHUNK;
            if (read(fd, (char *)buf, (int)n) != (int)n)
                return -1;
            for (i = 0; i < (unsigned int)n; i++)
                vbuf[i] = v[done + i];
            for (i = 0; i < (unsigned int)n; i++)
                if (vbuf[i] != buf[i])
                    bad++;
        }
    }
    return bad;
}

int main(int argc, char **argv)
{
    struct v7i h;
    unsigned int ver = 0;
    int c, fd, verify = 0, keep = 0, force = 0, secs = 5;
    long bad;

    while ((c = getopt(argc, argv, "t:vkF")) != -1) {
        switch (c) {
        case 't': secs = atoi(optarg); break;
        case 'v': verify = 1; break;
        case 'k': keep = 1; break;
        case 'F': force = 1; break;
        default:  usage();
        }
    }
    if (optind != argc - 1)
        usage();

    switch (v7_probe(&ver)) {
    case 1:
        break;
    case -1:
        fprintf(stderr, "v7show: Video Seven chip %04x, not a VEGA VGA\n", ver);
        return 1;
    default:
        fprintf(stderr, "v7show: no Video Seven adapter found\n");
        return 1;
    }

    fd = open(argv[optind], O_RDONLY);
    if (fd < 0) {
        perror(argv[optind]);
        return 1;
    }
    if (read_header(fd, &h) < 0) {
        close(fd);
        return 1;
    }
    printf("v7show: %ux%u, %u plane%s, mode %02x\n", h.width, h.height,
           h.planes, (h.planes == 1)? "": "s", h.mode);
    fflush(stdout);

    /*
     * An image taller than the monitor can scan would be drawn into a mode
     * that never appears, so say so instead of leaving a dark screen and a
     * program that claims success.
     */
    if (!force && h.height > V7_LOWRES_LINES && v7_monitor_lowres()) {
        fprintf(stderr, "v7show: monitor reports <= %u lines, this image"
                        " needs %u (-F to force)\n",
                V7_LOWRES_LINES, h.height);
        close(fd);
        return 1;
    }

    if (set_mode(h.mode) < 0) {
        close(fd);
        return 1;
    }

    bad = blit(fd, &h, verify);
    close(fd);

    if (h.planes > 1)
        restore_planes();
    if (!keep) {
        sleep(secs);
        set_mode(V7_MODE_80X25);
    }

    if (bad < 0) {
        fprintf(stderr, "v7show: image not drawn\n");
        return 1;
    }
    if (verify)
        printf("v7show: %ld byte%s differ on readback\n", bad,
               (bad == 1)? "": "s");
    return bad? 1: 0;
}
