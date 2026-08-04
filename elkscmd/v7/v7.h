/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Shared VEGA VGA access for the user space tools.
 *
 * ELKS runs in real mode, so a user program can reach the adapter's ports and
 * its BIOS directly.  That is why almost all of this lives here rather than in
 * the kernel: the kernel needs to know the card exists, and nothing more.
 *
 * Copyright (C) 2026 G Keet
 * Licensed under the GNU General Public License version 2, the same terms as
 * the ELKS kernel.
 */

#ifndef V7_H
#define V7_H

#include <arch/io.h>
#include <arch/video-v7.h>

static inline void v7_unlock(void)
{
    outb_p(V7_SR_ENABLE, V7_SEQ_INDEX);
    outb_p(V7_UNLOCK, V7_SEQ_DATA);
}

/*
 * Did the gate actually open?  The manual says SR6 reads back as 1 while the
 * extensions are enabled and 0 while they are not, so this distinguishes a
 * chip that accepted the unlock from one that ignored it - which otherwise
 * looks identical, because a locked extension index reads as FF and FF is a
 * plausible looking value for a register full of switch bits.
 */
static inline int v7_gate_open(void)
{
    outb_p(V7_SR_ENABLE, V7_SEQ_INDEX);
    return (inb_p(V7_SEQ_DATA) & 1) != 0;
}

static inline void v7_lock(void)
{
    outb_p(V7_SR_ENABLE, V7_SEQ_INDEX);
    outb_p(V7_LOCK, V7_SEQ_DATA);
}

static inline unsigned char v7_read(unsigned char reg)
{
    outb_p(reg, V7_SEQ_INDEX);
    return inb_p(V7_SEQ_DATA);
}

static inline void v7_write(unsigned char reg, unsigned char val)
{
    outb_p(reg, V7_SEQ_INDEX);
    outb_p(val, V7_SEQ_DATA);
}

/*
 * INT 10h with the registers the 6Fh functions use.  BP is pushed by hand
 * because a mode set is known to return with it altered on some BIOSes, and it
 * cannot be named in a clobber list while it may be the frame pointer.
 */
static inline void v7_bios(unsigned int *ax, unsigned int *bx, unsigned int *cx)
{
    unsigned int a = *ax, b = *bx, c = *cx;

    asm volatile ("push %%bp; int $0x10; pop %%bp"
                  : "+a" (a), "+b" (b), "+c" (c)
                  : : "dx", "si", "di", "memory", "cc");
    *ax = a;
    *bx = b;
    *cx = c;
}

/*
 * What class of monitor is on the analog connector?  6F01 answers with the
 * one the board detected, and its LOWRES bit means 200 lines or fewer: an
 * EGA, CGA or MDA class display, on which every mode above that will not sync
 * however willingly the chip programs it.
 */
static inline int v7_monitor_lowres(void)
{
    unsigned int ax = V7_BIOS_FUNC | V7_BIOS_GETINFO, bx = 0, cx = 0;

    v7_bios(&ax, &bx, &cx);
    return ((ax >> 8) & V7_INFO_LOWRES)? 1: 0;
}

/*
 * Is a VEGA VGA fitted?  The inquiry alone only says Video Seven, so the
 * version word decides: 8000h and up is the VEGA, below that is one of the
 * later single chip parts this driver does not claim to support.
 * Returns 1 for a VEGA, 0 for none, -1 for a Video Seven that is not a VEGA,
 * and stores the version word.
 */
static inline int v7_probe(unsigned int *version)
{
    unsigned int ax = V7_BIOS_FUNC | V7_BIOS_INQUIRE, bx = 0, cx = 0, ver;

    v7_bios(&ax, &bx, &cx);
    if (bx != V7_ID_BX)
        return 0;
    v7_unlock();
    ver = ((unsigned int)v7_read(V7_ER_VERHI) << 8) | v7_read(V7_ER_VERLO);
    v7_lock();
    if (version)
        *version = ver;
    return (ver >= V7_VER_VEGA_MIN)? 1: -1;
}

#endif
