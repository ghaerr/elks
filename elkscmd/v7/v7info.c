/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * v7info - report what a Video Seven VEGA VGA is and how it is strapped
 *
 * Reads nothing with a side effect except the switch strobe, which is what
 * the card's own BIOS does at power on.
 *
 * Copyright (C) 2026 G Keet
 * Licensed under the GNU General Public License version 2, the same terms as
 * the ELKS kernel.
 */

#include <stdio.h>
#include <stdlib.h>
#include "v7.h"

static void usage(void)
{
    fprintf(stderr, "usage: v7info [-d]\n");
    fprintf(stderr, "  -d  dump the raw extension registers\n");
    fprintf(stderr, "G Keet, 2026\n");
    exit(1);
}

/*
 * Raw dump, for telling a register that is really there from one that only
 * looks like it.  An index the chip does not implement reads back FF, and a
 * write to it is discarded, so comparing a dump either side of a write is the
 * only way to know whether the write meant anything.
 */
static const unsigned char dumpregs[] = {
    0x8E, 0x8F, 0xA5, 0xEA, 0xEB, 0xF7, 0xFC, 0xFE, 0xFF
};

static void dump(void)
{
    unsigned int i;

    v7_unlock();
    if (!v7_gate_open()) {
        v7_lock();
        printf("extensions did not unlock\n");
        return;
    }
    for (i = 0; i < sizeof(dumpregs); i++)
        printf("%02x=%02x ", dumpregs[i], v7_read(dumpregs[i]));
    v7_lock();
    printf("\n");
}

int main(int argc, char **argv)
{
    unsigned int ax, bx, cx, ver = 0;
    unsigned char sw, ff, mode;
    int gate;

    if (argc > 2)
        usage();
    if (argc == 2) {
        if (argv[1][0] != '-' || argv[1][1] != 'd' || argv[1][2])
            usage();
        dump();
        return 0;
    }

    switch (v7_probe(&ver)) {
    case 1:
        break;
    case -1:
        printf("v7info: Video Seven chip %04x, not a VEGA VGA\n", ver);
        return 1;
    default:
        printf("v7info: no Video Seven adapter (BIOS 6F00 did not answer)\n");
        return 1;
    }

    printf("chip      VEGA VGA, version %04x\n", ver);

    ax = V7_BIOS_FUNC | V7_BIOS_GETMEM;
    bx = cx = 0;
    v7_bios(&ax, &bx, &cx);
    printf("memory    %uK %s\n", ((ax >> 8) & V7_MEM_BANKS) << 8,
           ((ax >> 8) & V7_MEM_VRAM)? "VRAM": "DRAM");

    ax = V7_BIOS_FUNC | V7_BIOS_GETINFO;
    bx = cx = 0;
    v7_bios(&ax, &bx, &cx);
    printf("display   %s, %s\n",
           ((ax >> 8) & V7_INFO_MONO)? "monochrome": "colour",
           ((ax >> 8) & V7_INFO_LOWRES)? "<= 200 lines": "> 200 lines");

    ax = V7_BIOS_FUNC | V7_BIOS_GETMODE;
    bx = cx = 0;
    v7_bios(&ax, &bx, &cx);
    mode = (unsigned char)(ax & 0xFF);
    printf("mode      %02x, %u x %u\n", mode, bx, cx);

    v7_unlock();
    gate = v7_gate_open();
    v7_write(V7_ER_SWSTROBE, 0);        /* latch the switches into F7 */
    sw = v7_read(V7_ER_SWITCH);
    ff = v7_read(V7_ER_16BIT);
    v7_lock();

    if (!gate) {
        printf("\nextensions did not unlock: SR6 did not read back set.\n");
        printf("Switch and interface readings are not available on this\n");
        printf("card, so they are not shown rather than shown wrong.\n");
        return 0;
    }
    /*
     * A locked or absent extension register reads as FF, and FF is a
     * plausible looking answer for a byte of switches, so refuse to decode
     * it.  The same goes for the interface register: its bus width bit would
     * claim a 16-bit slot on a machine that has none.
     */
    if (sw == 0xFF && ff == 0xFF) {
        printf("\nswitch and interface registers both read FF, which is what\n");
        printf("an unimplemented extension index returns.  Not decoding them.\n");
        return 0;
    }

    printf("switches  %02x\n", sw);
    printf("  1-3      monitor strap %u\n", sw & V7_SW_MONITOR);
    printf("  4        16-bit I/O %s\n", (sw & V7_SW_16BIT_IO)? "on": "off");
    printf("  5        CGA/MGA emulation %s\n",
           (sw & V7_SW_CGA_EMU)? "on": "off");
    printf("  6        16-bit ROM %s\n", (sw & V7_SW_16BIT_ROM)? "on": "off");
    printf("  7        16-bit memory %s\n", (sw & V7_SW_16BIT_MEM)? "on": "off");
    printf("  8        %s\n", (sw & V7_SW_PURE_VGA)? "pure VGA mode":
           "Video Seven extensions enabled");
    printf("bus       %u-bit slot, fast write %s\n",
           (ff & V7_16BIT_BUS16)? 16: 8,
           (ff & V7_16BIT_FASTWRITE)? "on": "off");
    return 0;
}
