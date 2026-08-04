/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Video Seven VEGA VGA support
 *
 * The card works as a plain VGA without this file; the console drives it
 * either way.  What this adds is what the standard register set cannot reach:
 * confirmation of which Video Seven chip is fitted, how much display memory
 * it has, and how the board is strapped.
 *
 * Identification takes two steps because one is not enough.  BIOS 6F00
 * answering 'V7' says a Video Seven adapter is present but not which one, and
 * the extension registers of the later Headland parts are not all the same.
 * The version word at 8F:8E settles it, and only a VEGA is accepted here.
 *
 * Everything is a no-op on any other adapter, at a cost of one BIOS call at
 * boot.  The extensions gate is opened only around each access and closed
 * again: the manual asks for that, and under a multitasking kernel a stray
 * extension write would be very hard to attribute afterwards.
 *
 * Identification is split from reporting because the two want to happen at
 * different times.  v7_early_init() runs before console_init() so that a
 * v7=1 mode set is in force by the time the console reads the display width
 * out of the BIOS data area; v7_init() runs with the other character drivers
 * and only says what was found, which needs a console to say it to.
 *
 * Copyright (C) 2026 G Keet
 * Licensed under the GNU General Public License version 2, the same terms as
 * the ELKS kernel.
 */

#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/init.h>
#include <arch/io.h>
#include <arch/video-v7.h>

struct v7_info v7_info;

extern int v7_opts;                 /* /bootopts v7= options */

static int v7_identified;           /* the probe can run from either entry */

/*
 * BP is saved around the call by hand.  A mode set is the one INT 10h that is
 * known to return with BP altered on some BIOSes, and BP cannot be named in a
 * clobber list while the compiler may be using it as a frame pointer.  SI and
 * DI are only declared clobbered, which is enough for the compiler to keep
 * anything it cares about elsewhere.
 */
static void INITPROC v7_bios(unsigned int *ax, unsigned int *bx,
                             unsigned int *cx)
{
    unsigned int a = *ax, b = *bx, c = *cx;

    asm volatile ("push %%bp; int $0x10; pop %%bp"
                  : "+a" (a), "+b" (b), "+c" (c)
                  : : "dx", "si", "di", "memory", "cc");
    *ax = a;
    *bx = b;
    *cx = c;
}

void v7_unlock(void)
{
    outb_p(V7_SR_ENABLE, V7_SEQ_INDEX);
    outb_p(V7_UNLOCK, V7_SEQ_DATA);
}

void v7_lock(void)
{
    outb_p(V7_SR_ENABLE, V7_SEQ_INDEX);
    outb_p(V7_LOCK, V7_SEQ_DATA);
}

unsigned char v7_read(unsigned char reg)
{
    outb_p(reg, V7_SEQ_INDEX);
    return inb_p(V7_SEQ_DATA);
}

void v7_write(unsigned char reg, unsigned char val)
{
    outb_p(reg, V7_SEQ_INDEX);
    outb_p(val, V7_SEQ_DATA);
}

/*
 * Narrow down what is fitted, without printing: this runs before the console
 * exists.  6F00 is the card's own inquiry and nothing else answers it, so it
 * is what decides whether a mode set here is addressed to a Video Seven.
 */
static void INITPROC v7_identify(void)
{
    unsigned int ax, bx, cx;

    if (v7_identified)
        return;
    v7_identified = 1;

    ax = V7_BIOS_FUNC | V7_BIOS_INQUIRE;
    bx = cx = 0;
    v7_bios(&ax, &bx, &cx);
    if (bx != V7_ID_BX)
        return;                     /* not a Video Seven adapter */

    v7_unlock();
    v7_info.version = ((unsigned int)v7_read(V7_ER_VERHI) << 8) |
                      v7_read(V7_ER_VERLO);
    v7_lock();

    /*
     * Below 8000h is one of the single chip Headland parts.  Their extension
     * registers are close to the VEGA's but not identical, and none is fitted
     * here to test against, so say what was found and leave it alone.
     */
    if (v7_info.version >= V7_VER_VEGA_MIN)
        v7_info.present = 1;
}

/*
 * Put the card into VGA text mode, for /bootopts v7= bit 0.  6F05 is the
 * card's own mode set and takes the standard mode numbers as well as the
 * extended ones, so this is the same call the rest of the driver makes and
 * not a standard VGA one that happens to work here.
 */
static void INITPROC v7_set_vga_mode(void)
{
    unsigned int ax, bx, cx = 0;

    ax = V7_BIOS_FUNC | V7_BIOS_SETMODE;
    bx = V7_MODE_80X25;
    v7_bios(&ax, &bx, &cx);
}

/* Ask the card what geometry is in force, however it got there. */
static void INITPROC v7_read_mode(void)
{
    unsigned int ax, bx, cx;

    ax = V7_BIOS_FUNC | V7_BIOS_GETMODE;
    bx = cx = 0;
    v7_bios(&ax, &bx, &cx);
    v7_info.mode = (unsigned char)(ax & 0xFF);
    v7_info.cols = (unsigned char)bx;
    v7_info.rows = (unsigned char)cx;
}

/*
 * Called from start_kernel() ahead of console_init().  The mode set has to
 * happen here or not at all: the console reads the display width from the
 * BIOS data area as it starts and does not look again, so the same mode set
 * made later would change the screen without the console knowing.
 *
 * Without v7= this returns having done nothing, deliberately.  Everything the
 * driver needs can be found later on, from the usual place among the other
 * character drivers, and a boot with the option absent should not be running
 * BIOS calls at a point in the boot where there is not yet a console to
 * complain on if one misbehaves.
 */
void INITPROC v7_early_init(void)
{
    if (!(v7_opts & V7_OPT_VGAMODE))
        return;

    v7_identify();
    if (!v7_info.present)
        return;                     /* not a VEGA: leave the display alone */

    v7_set_vga_mode();
    v7_read_mode();
}

void INITPROC v7_init(void)
{
    unsigned int ax, bx, cx;
    unsigned char sw, ff;

    v7_identify();
    if (!v7_info.present) {
        if (v7_info.version)
            printk("v7: Video Seven chip %x is not a VEGA, unsupported\n",
                   v7_info.version);
        return;
    }

    /*
     * The switch and interface registers are documented for this family but
     * are not implemented on a VEGA: both read FF, which is what a missing
     * extension index returns.  FF is a plausible looking byte of switches
     * and its bus width bit would claim a 16-bit slot on an XT, so record
     * only what reads as real.
     */
    v7_unlock();
    sw = v7_read(V7_ER_SWITCH);
    ff = v7_read(V7_ER_16BIT);
    v7_lock();
    if (sw != 0xFF || ff != 0xFF) {
        v7_info.switches = sw;
        v7_info.have_switches = 1;
        v7_info.bus16 = (ff & V7_16BIT_BUS16)? 1: 0;
    }

    ax = V7_BIOS_FUNC | V7_BIOS_GETMEM;
    bx = cx = 0;
    v7_bios(&ax, &bx, &cx);
    v7_info.banks = (unsigned char)((ax >> 8) & V7_MEM_BANKS);
    v7_info.vram = ((ax >> 8) & V7_MEM_VRAM)? 1: 0;

    ax = V7_BIOS_FUNC | V7_BIOS_GETINFO;
    bx = cx = 0;
    v7_bios(&ax, &bx, &cx);
    v7_info.mono = ((ax >> 8) & V7_INFO_MONO)? 1: 0;
    v7_info.lowres = ((ax >> 8) & V7_INFO_LOWRES)? 1: 0;

    if (!v7_info.mode)              /* no early mode set, so ask now */
        v7_read_mode();

    printk("v7: VEGA VGA ver %x, %uK %s, %s %s monitor, mode %x %ux%u%s\n",
           v7_info.version, (unsigned int)v7_info.banks << 8,
           v7_info.vram? "VRAM": "DRAM",
           v7_info.mono? "mono": "colour",
           v7_info.lowres? "EGA/CGA": "VGA",
           v7_info.mode, v7_info.cols, v7_info.rows,
           (v7_opts & V7_OPT_VGAMODE)? " (v7=1)": "");
}
