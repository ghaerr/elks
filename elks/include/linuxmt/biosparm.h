#ifndef LX86_LINUXMT_BIOSPARM_H
#define LX86_LINUXMT_BIOSPARM_H

/* biosparm.h
 * Copyright (C) 1994 Yggdrasil Computing, Incorporated
 * 4880 Stevens Creek Blvd. Suite 205
 * San Jose, CA 95129-1034
 * USA
 * Tel (408) 261-6630
 * Fax (408) 261-6631
 *
 * This file is part of the Linux Kernel
 *
 * Linux is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Linux is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Linux; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Written by Ross Biro July 1994
 * Hacked up for Linux/8086 Alan Cox Feb 1996
 */

struct biosparms {
    unsigned short irq;		/* 0 */
    unsigned short ax;		/* 2 */
    unsigned short bx;		/* 4 */
    unsigned short cx;		/* 6 */
    unsigned short dx;		/* 8 */
    unsigned short si;		/* 10 */
    unsigned short di;		/* 12 */
    unsigned short bp;		/* 14 */
    unsigned short es;		/* 16 */
    unsigned short ds;		/* 18 */
    unsigned short fl;		/* 20 */
};

/* exported functions */
extern int call_bios(struct biosparms *);

#endif
