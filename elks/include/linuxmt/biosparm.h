#ifndef __LINUXMT_BIOSPARM_H
#define __LINUXMT_BIOSPARM_H

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
    unsigned short irq;         /* 0 */
    unsigned short ax;          /* 2 */
    unsigned short bx;          /* 4 */
    unsigned short cx;          /* 6 */
    unsigned short dx;          /* 8 */
    unsigned short si;          /* 10 */
    unsigned short di;          /* 12 */
    unsigned short bp;          /* 14 */
    unsigned short es;          /* 16 */
    unsigned short ds;          /* 18 */
    unsigned short fl;          /* 20 */
};

/* Useful defines for accessing the above structure. */
#define BD_AX bdt.ax
#define BD_BX bdt.bx
#define BD_CX bdt.cx
#define BD_DX bdt.dx
#define BD_SI bdt.si
#define BD_DI bdt.di
#define BD_BP bdt.bp
#define BD_ES bdt.es
#define BD_FL bdt.fl

#ifdef CONFIG_ARCH_PC98
#define BIOSHD_INT              0x1B
#define BIOSHD_RESET            0x0300
#define BIOSHD_WRITE            0xD500
#define BIOSHD_READ             0xD600
#define BIOSHD_DRIVE_PARMS      0x8400
#define BIOSHD_DEVICE_TYPE      0x1400
#define BIOSHD_MODESET          0x8E00
#else
#define BIOSHD_INT              0x13
#define BIOSHD_RESET            0x0000
#define BIOSHD_WRITE            0x0300
#define BIOSHD_READ             0x0200
#define BIOSHD_DRIVE_PARMS      0x0800
#endif

int call_bios(struct biosparms *);

void bios_disk_reset(int drive);
int bios_disk_rw(unsigned cmd, unsigned num_sectors, unsigned drive,
        unsigned cylinder, unsigned head, unsigned sector, unsigned seg, unsigned offset);
void bios_set_ddpt(int max_sectors);
void bios_copy_ddpt(void);
struct drive_infot;
void bios_switch_device98(int target, unsigned int device, struct drive_infot *drivep);

#endif
