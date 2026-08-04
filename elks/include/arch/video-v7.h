/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Video Seven VEGA VGA register definitions
 *
 * The VEGA is the original two-chip Video Seven VGA, which reports a version
 * word of 8000h or above.  The later Headland single-chip parts (HT208 in the
 * V7VGA, HT216) share the BIOS interface and much of the extension register
 * set, but they are not supported here: this driver covers the card that is
 * actually fitted and nothing else, so every register it touches is one that
 * can be tested.
 *
 * Sources: the V7 VGA Technical Reference Manual (6/30/88) for the BIOS 6Fh
 * interface and the extension gate, and the VGADOC Video 7 chipset notes for
 * which registers are common across the family.  Where the two could be read
 * as disagreeing about a register's meaning on a VEGA, this driver uses the
 * BIOS instead of the register.
 *
 *   http://www.bitsavers.org/components/video7/700-0242_V7_VGA_Technical_Reference_Manual_Jun88.pdf
 *
 * Documentation/text/videov7.txt has the rest.
 *
 * Copyright (C) 2026 G Keet
 * Licensed under the GNU General Public License version 2, the same terms as
 * the ELKS kernel.
 */

#ifndef __ARCH_VIDEO_V7_H
#define __ARCH_VIDEO_V7_H

/* Sequencer index/data, where the extension registers are reached */
#define V7_SEQ_INDEX    0x3C4
#define V7_SEQ_DATA     0x3C5

/* SR6 is the extensions gate: EA unlocks, AE locks */
#define V7_SR_ENABLE    0x06
#define V7_UNLOCK       0xEA
#define V7_LOCK         0xAE

/* Chip version: 8F is the high byte, 8E the low.  A VEGA reads 8000h or up. */
#define V7_ER_VERLO     0x8E
#define V7_ER_VERHI     0x8F
#define V7_VER_VEGA_MIN 0x8000

/* Extension registers this driver uses */
#define V7_ER_CURSOR_ATTR   0xA5    /* pointer enable, cursor blink/XOR */
#define V7_ER_SWSTROBE      0xEA    /* write strobes the switches into F7 */
#define V7_ER_SWITCH        0xF7    /* switch readback */
#define V7_ER_EMUL          0xEB    /* CGA/MDA/Hercules emulation control */
#define V7_ER_COMPAT        0xFC    /* extended attributes */
#define V7_ER_16BIT         0xFF    /* 16-bit interface and fast write */

/* A5, cursor attributes */
#define V7_CUR_NOBLINK      0x01
#define V7_CUR_XOR          0x08

/*
 * EB, emulation control.  Bit 7 traps writes to the CGA and monochrome
 * registers, and bit 6 forces a Hercules bitmap decode at B0000.  Both need
 * the Video Seven IOU, which is not on every board: where it is absent the
 * register reads FF and the card leaves B0000 alone, which is what lets a
 * real MDA or Hercules adapter share the machine.
 */
#define V7_EMUL_ENABLE      0x80
#define V7_EMUL_HERC_BITMAP 0x40

/* FC, compatibility control */
#define V7_COMPAT_EXTATTR   0x01    /* per character underline from plane 3 */

/* FF, 16-bit interface control */
#define V7_16BIT_MEM        0x01
#define V7_16BIT_IO         0x02
#define V7_16BIT_FASTWRITE  0x04
#define V7_16BIT_BUS16      0x80    /* read only: installed in a 16-bit slot */

/* CRTC register 9 bit 7: scan doubling */
#define V7_CRTC_MAXSCAN     0x09
#define V7_MAXSCAN_DOUBLE   0x80

/* BIOS interface, function 6Fh */
#define V7_BIOS_FUNC        0x6F00
#define V7_BIOS_INQUIRE     0x00    /* BX = 5637h if Video 7 extensions exist */
#define V7_BIOS_GETINFO     0x01    /* monitor information */
#define V7_BIOS_GETMODE     0x04    /* AL = mode, BX = columns, CX = rows */
#define V7_BIOS_SETMODE     0x05    /* BL = mode */
#define V7_BIOS_GETMEM      0x07    /* AH = 256K blocks, bit 7 = VRAM */

/*
 * 6F00 answers with the character pair 'V7' in BX.  An assembler puts the
 * first character in the high byte, so BH = 'V' and BL = '7'.
 */
#define V7_ID_BX            0x5637

/*
 * 6F01 AH, status bits.  LOWRES is the monitor class the board is strapped
 * for: a card driving its 9 pin TTL output into an EGA or CGA monitor reports
 * 200 lines or fewer, and every mode above that will not sync there however
 * willingly the chip programs it.
 */
#define V7_INFO_MONO        0x20
#define V7_INFO_LOWRES      0x10

/* Above this many scanlines a mode needs a VGA or multisync monitor. */
#define V7_LOWRES_LINES     200

/* 6F07 AH */
#define V7_MEM_BANKS        0x7F
#define V7_MEM_VRAM         0x80

/*
 * Text modes for 6F05.  The card's own mode set takes the standard VGA
 * numbers as well as its extended ones, which is why there is one path here
 * and not a BIOS call of a different shape for the IBM modes.  Mode 3 is the
 * VGA text mode proper, 720x400 in a 9x16 cell, and is what /bootopts v7=1
 * selects.
 */
#define V7_MODE_80X25       0x03    /* standard VGA colour text */
#define V7_MODE_80X25M      0x07    /* standard mono text */

/*
 * The extended text modes.  These are the ones a VEGA's own BIOS offers; the
 * tool asks the BIOS what it actually got rather than assuming, because which
 * of them a board supports depends on its memory and monitor strap.
 */
#define V7_MODE_80X43       0x40
#define V7_MODE_132X25      0x41
#define V7_MODE_132X43      0x42
#define V7_MODE_80X60       0x43
#define V7_MODE_100X60      0x44
#define V7_MODE_132X28      0x45

/*
 * Graphics modes.  12h is the standard VGA one and the only high resolution
 * mode here that does not depend on the board's memory or its monitor strap;
 * the 6xh modes are the card's own and have to be confirmed with 6F04 after
 * setting them rather than assumed.
 */
#define V7_MODE_640X480X16  0x12
#define V7_MODE_320X200X256 0x13
#define V7_MODE_752X410X16  0x60
#define V7_MODE_720X540X16  0x61
#define V7_MODE_800X600X16  0x62

/*
 * /bootopts v7= flag bits.
 *
 * Bit 0 is a mode set at boot, not a driver mode of operation, because that is
 * the only moment it can be one: console_init() reads the display width out of
 * the BIOS data area once and keeps it, so a mode set after the console has
 * started changes the screen without telling the console about it.  The same
 * switch in v7mode is therefore not the same thing, and neither replaces the
 * other.
 */
#define V7_OPT_VGAMODE      0x01    /* enter VGA text mode 3 before the console */

/* Board switches, strobed with EA and read from F7 */
#define V7_SW_MONITOR       0x07
#define V7_SW_16BIT_IO      0x08
#define V7_SW_CGA_EMU       0x10
#define V7_SW_16BIT_ROM     0x20
#define V7_SW_16BIT_MEM     0x40
#define V7_SW_PURE_VGA      0x80    /* set = pure VGA, clear = extensions on */

#ifndef __ASSEMBLER__

struct v7_info {
    unsigned char present;          /* 1 if a VEGA VGA was found */
    unsigned char mode;             /* text mode in force once the console starts */
    unsigned char cols;             /* columns the card reports for that mode */
    unsigned char rows;             /* rows, likewise */
    unsigned int version;           /* raw 8F:8E version word, 0 if no Video Seven */
    unsigned char banks;            /* 256K blocks of display memory */
    unsigned char vram;             /* 1 = VRAM, 0 = DRAM */
    unsigned char switches;         /* raw switch readback */
    unsigned char have_switches;    /* 0 = not implemented on this card */
    unsigned char mono;             /* monochrome display attached */
    unsigned char lowres;           /* 1 = monitor does 200 lines or fewer */
    unsigned char bus16;            /* in a 16-bit slot */
};

extern struct v7_info v7_info;

#endif /* __ASSEMBLER__ */
#endif /* __ARCH_VIDEO_V7_H */
