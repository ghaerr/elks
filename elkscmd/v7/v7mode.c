/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * v7mode - set a Video Seven text mode and drive the V7VGA extensions
 *
 * usage: v7mode [-l] [-m mode] [-d 0|1] [-u 0|1] [-c off|xor|noblink]
 *               [-f 0|1] [-r] [-F]
 *
 * The extended text modes are the reason this exists: 132x43 and 100x60 come
 * from the card's own BIOS, so the screen can be retimed and refilled without
 * anything in the kernel knowing about this card.
 *
 * What that does not do is move the kernel console with it.  console_init()
 * reads the display width out of the BIOS data area once as it starts and
 * hardcodes 25 rows, so a mode set from here changes the picture while the
 * console goes on addressing the screen it started with.  Anything other than
 * 80x25 is therefore for a program that draws the whole screen itself and puts
 * the mode back.  To bring the console up in a different mode, set it before
 * the console exists: /bootopts v7=1 enters VGA text mode 3 at boot, and the
 * console then reads its geometry from that.
 *
 * Every switch that changes an extension register can be put back with -r,
 * which restores the documented reset state.  The manual is firm that a BIOS
 * mode set does not reset the extensions, so leaving masked writes or colour
 * expansion enabled would make the next program that runs look very strange.
 *
 * Copyright (C) 2026 G Keet
 * Licensed under the GNU General Public License version 2, the same terms as
 * the ELKS kernel.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "v7.h"

struct modename {
    const char *name;
    unsigned char mode;
    unsigned short lines;       /* scanlines the display must be able to show */
    const char *desc;
};

static const struct modename modes[] = {
    { "vga",     V7_MODE_80X25,   200, "VGA text, what v7=1 sets at boot" },
    { "80x25",   V7_MODE_80X25,   200, "standard colour text" },
    { "80x25m",  V7_MODE_80X25M,  350, "standard mono text" },
    { "640x350m",0x0F,            350, "EGA mono graphics, MDA monitor" },
    { "640x200m",0x06,            200, "CGA mono graphics" },
    { "80x43",   V7_MODE_80X43,   350, "8x8 font" },
    { "132x25",  V7_MODE_132X25,  350, "8x14 font" },
    { "132x28",  V7_MODE_132X28,  392, "8x14 font" },
    { "132x43",  V7_MODE_132X43,  344, "8x8 font" },
    { "80x60",   V7_MODE_80X60,   480, "8x8 font" },
    { "100x60",  V7_MODE_100X60,  480, "8x8 font" },
    { "640x480", V7_MODE_640X480X16, 480, "16 colour graphics, standard VGA" },
    { "320x200", V7_MODE_320X200X256,200, "256 colour graphics, standard VGA" },
    { "752x410", V7_MODE_752X410X16, 410, "16 colour graphics" },
    { "720x540", V7_MODE_720X540X16, 540, "16 colour graphics" },
    { "800x600", V7_MODE_800X600X16, 600, "16 colour graphics" },
};
#define NMODES ((int)(sizeof(modes) / sizeof(modes[0])))

static void usage(void)
{
    int i;

    fprintf(stderr, "usage: v7mode [-l] [-m mode] [-d 0|1] [-u 0|1]\n");
    fprintf(stderr, "              [-c off|xor|noblink] [-f 0|1] [-r] [-F]\n");
    fprintf(stderr, "  -l  list modes        -m  set a text mode\n");
    fprintf(stderr, "  -d  scan doubling     -u  extended underline"
                    " attributes\n");
    fprintf(stderr, "  -c  text cursor       -f  fast write\n");
    fprintf(stderr, "  -r  put the extension registers back to reset state\n");
    fprintf(stderr, "  -F  set a mode the detected monitor cannot scan\n");
    fprintf(stderr, "modes:");
    for (i = 0; i < NMODES; i++)
        fprintf(stderr, " %s", modes[i].name);
    fprintf(stderr, "\nG Keet, 2026\n");
    exit(1);
}

static void list_modes(void)
{
    int i;

    for (i = 0; i < NMODES; i++)
        printf("  %-8s mode %02x  %s\n", modes[i].name, modes[i].mode,
               modes[i].desc);
}

static int force;

static int set_mode(const char *name)
{
    unsigned int ax, bx, cx;
    int i;

    for (i = 0; i < NMODES; i++)
        if (!strcmp(name, modes[i].name))
            break;
    if (i == NMODES) {
        fprintf(stderr, "v7mode: unknown mode %s\n", name);
        return -1;
    }
    /*
     * The board reports the monitor it detected, and a mode taller than that
     * monitor can scan is not a picture nobody likes, it is no picture at
     * all.  Refuse it rather than blank the screen of anyone who has to walk
     * to the machine to see what happened.  -F is for a strap that lies.
     */
    if (!force && modes[i].lines > V7_LOWRES_LINES && v7_monitor_lowres()) {
        fprintf(stderr, "v7mode: monitor reports <= %u lines, %s needs %u"
                        " (-F to force)\n", V7_LOWRES_LINES, name,
                modes[i].lines);
        return -1;
    }
    /*
     * 6F05 sets both the standard and the extended modes, so there is one
     * path here rather than a special case for the IBM numbers.
     */
    ax = V7_BIOS_FUNC | V7_BIOS_SETMODE;
    bx = modes[i].mode;
    cx = 0;
    v7_bios(&ax, &bx, &cx);

    ax = V7_BIOS_FUNC | V7_BIOS_GETMODE;
    bx = cx = 0;
    v7_bios(&ax, &bx, &cx);
    printf("v7mode: mode %02x, %u x %u\n", ax & 0xFF, bx, cx);

    /*
     * Say this rather than let it be discovered.  The console keeps the
     * geometry it started with, so anything but 80x25 leaves it addressing a
     * screen that is no longer there.
     */
    if (bx != 80 || cx != 25)
        printf("v7mode: the console still thinks it is 80x25;"
               " use /bootopts v7=1 for a boot mode\n");
    return 0;
}

/*
 * Scan doubling lives in the CRTC, not the extensions, so it needs the CRTC
 * address port - which is 3B4 on a mono display and 3D4 on a colour one.
 * Bit 0 of the misc output register says which, and reading it is the only
 * reliable way when the mode may just have changed.
 */
static unsigned int crtc_port(void)
{
    return (inb_p(0x3CC) & 1)? 0x3D4: 0x3B4;
}

static void scan_double(int on)
{
    unsigned int port = crtc_port();
    unsigned char v;

    outb_p(V7_CRTC_MAXSCAN, port);
    v = inb_p(port + 1);
    if (on)
        v |= V7_MAXSCAN_DOUBLE;
    else
        v &= (unsigned char)~V7_MAXSCAN_DOUBLE;
    outb_p(V7_CRTC_MAXSCAN, port);
    outb_p(v, port + 1);
    outb_p(V7_CRTC_MAXSCAN, port);
    if (inb_p(port + 1) != v) {
        fprintf(stderr, "v7mode: scan doubling did not take at crtc %x\n",
                port);
        return;
    }
    /*
     * The manual is explicit that this bit changes how data is displayed and
     * not the timing, so on its own it makes the picture taller.  Retiming
     * the CRTC for the doubled line count is a separate job.
     */
    printf("v7mode: scan doubling %s (timing unchanged, see the manual)\n",
           on? "on": "off");
}

/*
 * Writing an extension index while the gate is shut does not write an
 * extension register, it writes whatever standard sequencer register shares
 * that index - so confirm the gate before touching anything.
 */
static int gate_or_warn(void)
{
    v7_unlock();
    if (v7_gate_open())
        return 1;
    v7_lock();
    fprintf(stderr, "v7mode: extensions did not unlock, refusing to write\n");
    return 0;
}

/*
 * Write an extension register and prove it took.  Several indices the family
 * documents are simply absent on a VEGA: they read FF and swallow writes, so
 * a command that only wrote would report success having changed nothing.
 * Read the register back and say so when it did not stick.
 */
static int reg_live(unsigned char reg, const char *what)
{
    /*
     * An absent extension index reads FF and swallows writes.  Reading back
     * after the write does not catch it: these registers reset to 0, so a
     * command that sets a bit turns FF into FF and the readback agrees with
     * itself.  Test the register before touching it instead.
     *
     * The one place this could misjudge a working card is a 16-bit board with
     * every bit of the interface register set, which would also read FF.
     * Nothing to hand can be that, so the trade is worth it.
     */
    if (v7_read(reg) != 0xFF)
        return 1;
    fprintf(stderr, "v7mode: %s is not implemented on this card"
                    " (reg %02x reads FF)\n", what, reg);
    return 0;
}

static int write_verify(unsigned char reg, unsigned char val, const char *what)
{
    unsigned char got;

    if (!reg_live(reg, what))
        return 0;
    v7_write(reg, val);
    got = v7_read(reg);
    if (got == val)
        return 1;
    fprintf(stderr, "v7mode: %s did not take (reg %02x holds %02x,"
                    " wrote %02x)\n", what, reg, got, val);
    return 0;
}

static void ext_attr(int on)
{
    unsigned char v;

    if (!gate_or_warn())
        return;
    v = v7_read(V7_ER_COMPAT);
    if (on)
        v |= V7_COMPAT_EXTATTR;
    else
        v &= (unsigned char)~V7_COMPAT_EXTATTR;
    if (!write_verify(V7_ER_COMPAT, v, "extended underline attributes")) {
        v7_lock();
        return;
    }
    v7_lock();
    printf("v7mode: extended underline attributes %s\n", on? "on": "off");
}

static void cursor_mode(const char *how)
{
    unsigned char v;

    if (!gate_or_warn())
        return;
    v = v7_read(V7_ER_CURSOR_ATTR);
    v &= (unsigned char)~(V7_CUR_XOR | V7_CUR_NOBLINK);
    if (!strcmp(how, "xor"))
        v |= V7_CUR_XOR;
    else if (!strcmp(how, "noblink"))
        v |= V7_CUR_NOBLINK;
    else if (strcmp(how, "off")) {
        v7_lock();
        fprintf(stderr, "v7mode: cursor must be off, xor or noblink\n");
        return;
    }
    if (!write_verify(V7_ER_CURSOR_ATTR, v, "cursor attributes")) {
        v7_lock();
        return;
    }
    v7_lock();
    printf("v7mode: text cursor %s\n", how);
}

/*
 * Fast write releases the CPU as soon as the address and data are latched, so
 * a write costs no wait states unless another access is already pending.  It
 * needs the 16-bit interface, which needs a 16-bit slot, so refuse it on an
 * 8-bit bus rather than set a bit that cannot do anything.
 */
static void fast_write(int on)
{
    unsigned char v;

    if (!gate_or_warn())
        return;
    v = v7_read(V7_ER_16BIT);
    if (on && !(v & V7_16BIT_BUS16)) {
        v7_lock();
        fprintf(stderr, "v7mode: 8-bit slot, fast write would do nothing\n");
        return;
    }
    if (on)
        v |= V7_16BIT_FASTWRITE;
    else
        v &= (unsigned char)~V7_16BIT_FASTWRITE;
    if (!write_verify(V7_ER_16BIT, v, "the 16-bit interface register")) {
        v7_lock();
        return;
    }
    v7_lock();
    printf("v7mode: fast write %s\n", on? "on": "off");
}

/* Put back the documented reset state of everything this program can change. */
static void reset_extensions(void)
{
    int n;

    if (!gate_or_warn())
        return;
    n = write_verify(V7_ER_CURSOR_ATTR, 0, "cursor attributes");
    n += write_verify(V7_ER_COMPAT, 0, "extended attributes");
    v7_lock();
    printf("v7mode: %d of 2 extension registers reset\n", n);
}

int main(int argc, char **argv)
{
    unsigned int ver = 0;
    int c;

    if (argc < 2)
        usage();
    if (!strcmp(argv[1], "-l")) {
        list_modes();
        return 0;
    }
    switch (v7_probe(&ver)) {
    case 1:
        break;
    case -1:
        fprintf(stderr, "v7mode: Video Seven chip %04x, not a VEGA VGA\n",
                ver);
        return 1;
    default:
        fprintf(stderr, "v7mode: no Video Seven adapter found\n");
        return 1;
    }

    /*
     * -F is looked for before the options are walked, because getopt hands
     * them over in the order they were typed and "-m 800x600 -F" should mean
     * the same thing as "-F -m 800x600".
     */
    for (c = 1; c < argc; c++)
        if (!strcmp(argv[c], "-F"))
            force = 1;

    while ((c = getopt(argc, argv, "lm:d:u:c:f:rF")) != -1) {
        switch (c) {
        case 'l': list_modes(); break;
        case 'm': if (set_mode(optarg) < 0) return 1; break;
        case 'd': scan_double(atoi(optarg)); break;
        case 'u': ext_attr(atoi(optarg)); break;
        case 'c': cursor_mode(optarg); break;
        case 'f': fast_write(atoi(optarg)); break;
        case 'r': reset_extensions(); break;
        case 'F': force = 1; break;
        default:  usage();
        }
    }
    return 0;
}
