/*
 * bioshd.c - ELKS floppy and hard disk driver - uses BIOS
 *
 * Driver Copyright (C) 1994 Yggdrasil Computing, Inc.
 * Extended and modified for Linux 8086 by Alan Cox.
 *
 * Originally from
 * doshd.c copyright (C) 1994 Yggdrasil Computing, Incorporated
 * 4880 Stevens Creek Blvd. Suite 205
 * San Jose, CA 95129-1034
 * USA
 *
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
 * Enhanced by Greg Haerr Oct 2020: add track cache, XT fixes, custom DDPT
 */

#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/genhd.h>
#include <linuxmt/biosparm.h>
#include <linuxmt/major.h>
#include <linuxmt/bioshd.h>
#include <linuxmt/fs.h>
#include <linuxmt/string.h>
#include <linuxmt/mm.h>
#include <linuxmt/memory.h>
#include <linuxmt/config.h>
#include <linuxmt/debug.h>
#include <linuxmt/timer.h>

#include <arch/hdreg.h>
#include <arch/io.h>
#include <arch/segment.h>
#include <arch/system.h>
#include <arch/irq.h>
#include <arch/ports.h>

#define DEBUG_PROBE     0       /* =1 to display more floppy probing information */
#define FORCE_PROBE     0       /* =1 to force floppy probing */

/* the following must match with /dev minor numbering scheme*/
#define NUM_MINOR	32	/* max minor devices per drive*/
#define MINOR_SHIFT	5	/* =log2(NUM_MINOR) shift to get drive num*/
#define NUM_DRIVES	8	/* =256/NUM_MINOR max number of drives*/
#define DRIVE_FD0	4	/* first floppy drive =NUM_DRIVES/2*/
#define DRIVE_FD1	5	/* second floppy drive*/
#define DRIVE_FD2	6	/* PC98 only*/
#define DRIVE_FD3	7	/* PC98 only*/

#define MAJOR_NR BIOSHD_MAJOR
#define BIOSDISK

#include "blk.h"

#ifdef CONFIG_ARCH_IBMPC
#define MAXDRIVES	2	/* max floppy drives*/
#endif

#ifdef CONFIG_ARCH_PC98
#define MAXDRIVES	4	/* max floppy drives*/
#endif

/* comment out following line for single-line drive info summary*/
#define PRINT_DRIVE_INFO	NUM_DRIVES

struct elks_disk_parms {
    __u16 track_max;		/* number of tracks, little-endian */
    __u8 sect_max;		/* number of sectors per track */
    __u8 head_max;		/* number of disk sides/heads */
    __u8 size;			/* size of parameter block (everything before
				   this point) */
    __u8 marker[2];		/* should be "eL" */
} __attribute__((packed));

static int bioshd_initialized = 0;
static struct biosparms bdt;

/* Useful defines for accessing the above structure. */
#define CARRY_SET (bdt.fl & 0x1)
#define BD_AX bdt.ax
#define BD_BX bdt.bx
#define BD_CX bdt.cx
#define BD_DX bdt.dx
#define BD_SI bdt.si
#define BD_DI bdt.di
#define BD_BP bdt.bp
#define BD_ES bdt.es
#define BD_FL bdt.fl

static struct drive_infot drive_info[NUM_DRIVES];

/* This makes probing order more logical and
 * avoids a few senseless seeks in some cases
 */

static struct hd_struct hd[NUM_DRIVES << MINOR_SHIFT];	/* partitions start, size*/

static int access_count[NUM_DRIVES];	/* for invalidating buffers/inodes*/

static int bioshd_sizes[NUM_DRIVES << MINOR_SHIFT];	/* used only with BDEV_SIZE_CHK*/

struct drive_infot fd_types[] = {	/* AT/PS2 BIOS reported floppy formats*/
    {40,  9, 2, 512, 0},
    {80, 15, 2, 512, 1},
    {80,  9, 2, 512, 2},
    {80, 18, 2, 512, 3},
    {80, 36, 2, 512, 4},
#ifdef CONFIG_ARCH_PC98
    {77,  8, 2, 1024,5},
#endif
};

#ifdef CONFIG_ARCH_PC98
unsigned char hd_drive_map[NUM_DRIVES] = {/* BIOS drive mappings*/
    0xA0, 0xA1, 0xA2, 0xA3,		/* hda, hdb */
#ifdef CONFIG_IMG_FD1232
    0x90, 0x91, 0x92, 0x93		/* fd0, fd1 */
#else
    0x30, 0x31, 0x32, 0x33		/* fd0, fd1 */
#endif
};
#else
unsigned char hd_drive_map[NUM_DRIVES] = {/* BIOS drive mappings*/
    0x80, 0x81, 0x82, 0x83,		/* hda, hdb */
    0x00, 0x01, 0x02, 0x03		/* fd0, fd1 */
};
#endif

static int _fd_count = 0;  		/* number of floppy disks */
static int _hd_count = 0;  		/* number of hard disks */

#define SPT		4	/* DDPT offset of sectors per track*/
static unsigned char DDPT[14];	/* our copy of diskette drive parameter table*/
unsigned long __far *vec1E = _MK_FP(0, 0x1E << 2);

static int bioshd_ioctl(struct inode *, struct file *, unsigned int, unsigned int);
static int bioshd_open(struct inode *, struct file *);
static void bioshd_release(struct inode *, struct file *);
static void bioshd_geninit(void);
static void set_cache_invalid(void);

static struct gendisk bioshd_gendisk = {
    MAJOR_NR,			/* Major number */
    "hd",			/* Major name */
    MINOR_SHIFT,		/* Bits to shift to get real from partition */
    1 << MINOR_SHIFT,		/* Number of partitions per real */
    NUM_DRIVES,			/* maximum number of real */
    bioshd_geninit,		/* init function */
    hd,				/* hd struct */
    bioshd_sizes,		/* sizes not blocksizes */
    0,				/* number */
    (void *) drive_info,	/* internal */
    NULL			/* next */
};

struct drive_infot *last_drive;	/* set to last drivep-> used in read/write */
static struct drive_infot *cache_drive;

static void set_cache_invalid(void)
{
	cache_drive = NULL;
}

static int bios_disk_rw(unsigned cmd, unsigned num_sectors, unsigned drive,
	unsigned cylinder, unsigned head, unsigned sector, unsigned seg, unsigned offset)
{
#ifdef CONFIG_ARCH_PC98
	BD_AX = cmd | drive;
    if (((0xF0 & drive) == 0x80) || ((0xF0 & drive) == 0xA0)) {
	BD_BX = (unsigned int) (num_sectors << 9);
	BD_CX = cylinder;
	BD_DX = (head << 8) | ((sector - 1) & 0xFF);
    }
    else {
	if ((0xF0 & drive) == 0x90) {
	    BD_BX = (unsigned int) (num_sectors << 10);
	    BD_CX = (3 << 8) | cylinder;
	}
	else {
	    BD_BX = (unsigned int) (num_sectors << 9);
	    BD_CX = (2 << 8) | cylinder;
	}
	BD_DX = (head << 8) | sector;
    }
	BD_ES = seg;
	BD_BP = offset;
#else
	BD_AX = cmd | num_sectors;
	BD_CX = (unsigned int) ((cylinder << 8) | ((cylinder >> 2) & 0xc0) | sector);
	BD_DX = (head << 8) | drive;
	BD_ES = seg;
	BD_BX = offset;
#endif
	return call_bios(&bdt);
}

#ifdef CONFIG_BLK_DEV_BHD
/* This function checks to see which hard drives are active and sets up the
 * drive_info[] table for them.  Ack this is darned confusing...
 */
static unsigned short int INITPROC bioshd_gethdinfo(void) {
    int drive, ndrives = 0;
    register struct drive_infot *drivep = &drive_info[0];

#ifdef CONFIG_ARCH_PC98
    int ide_drives = 0;
    int scsi_id;
    int device_type;
    int call_bios_rvalue;

    /* IDE */
    for (drive = 0; drive < 4; drive++) {
	if (peekb(0x55D,0) & (1 << drive)) {
	    BD_AX = BIOSHD_MODESET | (drive + 0x80);
	    BD_ES = BD_DI = BD_SI = 0;
	    call_bios(&bdt);
	    hd_drive_map[ide_drives++] = drive + 0x80;
	}
    }
    if (ide_drives > 0)
	printk("bioshd: Detected IDE hd.\n");
    ndrives = ide_drives;

    /* SCSI */
    if (ndrives < 4) {
	for (scsi_id = 0; scsi_id < 7; scsi_id++) {
	    BD_AX = BIOSHD_DRIVE_PARMS | (scsi_id + 0xA0);
	    BD_ES = BD_DI = BD_SI = 0;
	    call_bios_rvalue = call_bios(&bdt);
	    if ((call_bios_rvalue == 0) && (BD_DX & 0xff)) {
	        BD_AX = BIOSHD_DEVICE_TYPE | (scsi_id + 0xA0);
	        BD_ES = BD_DI = BD_SI = 0;
	        BD_BX = 0;
	        call_bios(&bdt);
	        device_type = BD_BX & 0xf; /* device_type = 0 for Hard Disk */
	        if (device_type == 0)
	            hd_drive_map[ndrives++] = scsi_id + 0xA0;
	    }
	    if (ndrives >= 4) break;
	}
    }
    if (ndrives > ide_drives)
	printk("bioshd: Detected SCSI hd.\n");
#else
    BD_AX = BIOSHD_DRIVE_PARMS;
    BD_DX = 0x80;		/* query hard drives only*/
    BD_ES = BD_DI = BD_SI = 0;	/* guard against BIOS bugs*/
    if (!call_bios(&bdt))
	ndrives = BD_DX & 0xff;
    else
	debug_bios("bioshd: get_drive_parms fail on hd\n");
#endif
    if (ndrives > NUM_DRIVES/2)
	ndrives = NUM_DRIVES/2;

    for (drive = 0; drive < ndrives; drive++) {
#ifdef CONFIG_ARCH_PC98
	BD_AX = BIOSHD_DRIVE_PARMS | hd_drive_map[drive];
#else
	BD_AX = BIOSHD_DRIVE_PARMS;
	BD_DX = drive + 0x80;
#endif
	BD_ES = BD_DI = BD_SI = 0;	/* guard against BIOS bugs*/
	if (call_bios(&bdt) == 0) {
#ifdef CONFIG_ARCH_PC98
	    drivep->heads = BD_DX >> 8;
	    drivep->sectors = BD_DX & 0xff;
	    drivep->cylinders = BD_CX;
#else
	    drivep->heads = (BD_DX >> 8) + 1;
	    drivep->sectors = BD_CX & 0x3f;
	    /* NOTE: some BIOS may underreport cylinders by 1*/
	    drivep->cylinders = (((BD_CX & 0xc0) << 2) | (BD_CX >> 8)) + 1;
#endif
	    drivep->fdtype = -1;
	    drivep->sector_size = 512;
	    printk("bioshd: hd%c BIOS CHS %u,%d,%d\n", 'a'+drive, drivep->cylinders,
		drivep->heads, drivep->sectors);
	}
#ifdef CONFIG_IDE_PROBE
	if (sys_caps & CAP_HD_IDE) {		/* Normally PC/AT or higher */
	    if (!get_ide_data(drive, drivep)) {	/* get CHS from the drive itself */
		/* sanity checks already done, accepting data */
		printk("bioshd: hd%c  IDE CHS %d,%d,%d\n", 'a'+drive, drivep->cylinders,
		drivep->heads, drivep->sectors);
	    }
	}
#endif
	drivep++;
    }
    return ndrives;
}
#endif

#ifdef CONFIG_BLK_DEV_BFD_HARD
/* hard-coded floppy configuration*/
static unsigned short int INITPROC bioshd_getfdinfo(void)
{
/* Set this to match your system. Currently it's set to a two drive system:
 *
 *		720KB as /dev/fd0
 *	and	720KB as /dev/fd1
 *
 * ndrives is number of drives in your system (either 0, 1 or 2)
 */

    int ndrives = MAXDRIVES;

/* drive_info[] should be set *only* for existing drives;
 * comment out drive_info lines if you don't need them
 * (e.g. you have less than 2 drives)
 *
 * Enter type 4 in fd_types' brackets for unknown drive type
 * Otherwise use floppy drive type table below:
 *
 *	Type	Format
 *	~~~~	~~~~~~
 *	  0	360 KB
 *	  1	1.2 MB
 *	  2	720 KB
 *	  3	1.44 MB
 *	  4	2.88 MB or Unknown
 *	  5	1.232 MB (PC98 1K sectors)
 *
 * Warning: drive will be reported as 2880 KB at bootup if you've set it
 * as unknown (4). Floppy probe will detect correct floppy format at each
 * change so don't bother with that
 */

#ifdef CONFIG_ARCH_PC98
#if defined(CONFIG_IMG_FD1232)
    drive_info[DRIVE_FD0] = fd_types[5];
    drive_info[DRIVE_FD1] = fd_types[5];
    drive_info[DRIVE_FD2] = fd_types[5];
    drive_info[DRIVE_FD3] = fd_types[5];
#elif defined(CONFIG_IMG_FD1440)
    drive_info[DRIVE_FD0] = fd_types[3];
    drive_info[DRIVE_FD1] = fd_types[3];
    drive_info[DRIVE_FD2] = fd_types[3];
    drive_info[DRIVE_FD3] = fd_types[3];
#endif
#endif

#ifdef CONFIG_ARCH_IBMPC
    drive_info[DRIVE_FD0] = fd_types[2];	/*  /dev/fd0    */
    drive_info[DRIVE_FD1] = fd_types[2];	/*  /dev/fd1    */
#endif

    return ndrives;
}

#elif defined(CONFIG_BLK_DEV_BFD)

/* use BIOS to query floppy configuration*/
static unsigned short int INITPROC bioshd_getfdinfo(void)
{
    register struct drive_infot *drivep = &drive_info[DRIVE_FD0];
    int drive, ndrives = 0;

#ifndef CONFIG_ROMCODE
    /*
     * The INT 13h floppy query will fail on IBM XT v1 BIOS and earlier,
     * so default to # drives from the BIOS data area at 0x040:0x0010 (INT 11h).
     */
    unsigned char equip_flags = peekb(0x10, 0x40);
    if (equip_flags & 0x01)
	ndrives = (equip_flags >> 6) + 1;
#endif

#ifdef CONFIG_ARCH_PC98
    for (drive = 0; drive < 4; drive++) {
	if (peekb(0x55C,0) & (1 << drive)) {
#ifdef CONFIG_IMG_FD1232
	    hd_drive_map[DRIVE_FD0 + ndrives] = drive + 0x90;
	    *drivep = fd_types[5];
#else
	    hd_drive_map[DRIVE_FD0 + ndrives] = drive + 0x30;
	    *drivep = fd_types[3];
#endif
	    ndrives++;	/* floppy drive count*/
	    drivep++;
	}
    }
#else
    /* Floppy query may fail if not PC/AT */
    BD_AX = BIOSHD_DRIVE_PARMS;
    BD_DX = 0;                  /* query floppies only*/
    BD_ES = BD_DI = BD_SI = 0;	/* guard against BIOS bugs*/
    if (!call_bios(&bdt))
        ndrives = BD_DX & 0xff;	/* floppy drive count*/
    else
        printk("fd: no bios get drives, ndrives %d\n", ndrives);

    /* set drive type for floppies*/
    for (drive = 0; drive < ndrives; drive++) {
	/*
	 * If type cannot be determined using BIOSHD_DRIVE_PARMS,
	 * set drive type to 1.4MM on AT systems, and 360K for XT.
	 */
        BD_AX = BIOSHD_DRIVE_PARMS;
        BD_DX = drive;
        BD_ES = BD_DI = BD_SI = 0;      /* guard against BIOS bugs*/
        if (!call_bios(&bdt))           /* returns drive type in BL*/
            *drivep = fd_types[(BD_BX & 0xFF) - 1];
        else
            *drivep = fd_types[(sys_caps & CAP_PC_AT) ? 3 : 0];
        drivep++;
    }
#endif
    return ndrives;
}
#endif

static void bioshd_release(struct inode *inode, struct file *filp)
{
    int target;
    kdev_t dev = inode->i_rdev;

    sync_dev(dev);
    target = DEVICE_NR(dev);
    access_count[target]--;
    if ((target >= DRIVE_FD0) && (access_count[target] == 0)) {
	invalidate_inodes(dev);
	invalidate_buffers(dev);
    }
}

/* set our DDPT sectors per track value*/
static void set_ddpt(int max_sectors)
{
	DDPT[SPT] = (unsigned char) max_sectors;
}

/* get the diskette drive parameter table from INT 1E and point to our RAM copy of it*/
static void copy_ddpt(void)
{
	unsigned long oldvec = *vec1E;

	/* We want to prevent the BIOS from accidentally doing a "multitrack"
	 * floppy read --- and wrapping around from one head to the next ---
	 * when ELKS only wants to read from a single track.
	 *
	 * (E.g. if DDPT SPT = 9, and a disk has 18 sectors per track, and we
	 * want to read sectors 9--10 from track 0, side 0, then the BIOS may
	 * read sector 9 from track 0, side 0, followed by sector 1 from track
	 * 0, side 1, which will be wrong.)
	 *
	 * To prevent this, we set the DDPT SPT field to the actual sector
	 * count per track in the detected disk geometry.  The DDPT SPT
	 * should never be smaller than the actual SPT, but it can be larger.
	 *
	 * Rather than issue INT 13h function 8 (Get Drive Parameters, not implemented
	 * on IBM XT BIOS v1 and earlier) to get an accurate DDPT, just copy the original
	 * DDPT to RAM, where the sectors per track value will be modified before each
	 * INT 13h function 2/3 (Read/Write Disk Sectors).
	 * Using a patched DDPT also eliminates the need for a seperate fix for #39/#44.
	 */
	fmemcpyw(DDPT, _FP_SEG(DDPT), (void *)(unsigned)oldvec, _FP_SEG(oldvec),
		sizeof(DDPT)/2);
	debug_bios("bioshd: DDPT vector %x:%x SPT %d\n", _FP_SEG(oldvec), (unsigned)oldvec, DDPT[SPT]);
	*vec1E = (unsigned long)(void __far *)DDPT;
}

/* As far as I can tell this doesn't actually work, but we might
 * as well try it -- Some XT controllers are happy with it.. [AC]
 */

static void reset_bioshd(int drive)
{
#ifdef CONFIG_ARCH_PC98
    BD_AX = BIOSHD_RESET | drive;
#else
    BD_AX = BIOSHD_RESET;
    BD_DX = drive;
#endif
    call_bios(&bdt);
    /* ignore errors with carry set*/
}

/* map drives */
static void map_drive(int *drive)
{
	*drive = hd_drive_map[*drive];
}

#ifdef CONFIG_ARCH_PC98
/* switch device */
static void switch_device98(int target, unsigned char device, struct drive_infot *drivep)
{
    hd_drive_map[target + DRIVE_FD0] = (device | (hd_drive_map[target + DRIVE_FD0] & 0x0F));
    if (device == 0x30)
	*drivep = fd_types[3];	/* 1.44 MB */
    else if (device == 0x90)
	*drivep = fd_types[5];	/* 1.232 MB */
}
#endif

static int read_sector(int drive, int cylinder, int sector)
{
    int count = 2;		/* one retry on probe or boot sector read */

#ifdef CONFIG_ARCH_PC98
    drive += DRIVE_FD0;
    map_drive(&drive);
#endif

    set_cache_invalid();
    do {
	set_irq();
	set_ddpt(36);		/* set to large value to avoid BIOS issues*/
	if (!bios_disk_rw(BIOSHD_READ, 1, drive, cylinder, 0, sector, DMASEG, 0))
	    return 0;			/* everything is OK */
	reset_bioshd(drive);
    } while (--count > 0);
    return 1;			/* error */
}

#ifdef CONFIG_BLK_DEV_BFD
static void probe_floppy(int target, struct hd_struct *hdp)
{
/* Check for disk type */

/* I guess it works now as it should. Tested under dosemu with 720Kb,
 * 1.2 MB and 1.44 MB floppy image and works fine - Blaz Antonic
 */

    if (target >= DRIVE_FD0) {		/* the floppy drives */
	register struct drive_infot *drivep = &drive_info[target];

/* probing range can be easily extended by adding more values to these
 * two lists and adjusting for loop' parameters in line 433 and 446 (or
 * somewhere near)
 */
#ifdef CONFIG_ARCH_PC98
	static unsigned char sector_probe[2] = { 8, 18 };
	static unsigned char track_probe[2] = { 77, 80 };
#else
	static unsigned char sector_probe[5] = { 8, 9, 15, 18, 36 };
	static unsigned char track_probe[2] = { 40, 80 };
#endif
	int count, found_PB = 0;

	target &= MAXDRIVES - 1;

#if !FORCE_PROBE
	/* Try to look for an ELKS or DOS parameter block in the first sector.
	 * If it exists, we can obtain the disk geometry from it.
	 */
	if (!read_sector(target, 0, 1)) {
	    struct elks_disk_parms __far *parms = _MK_FP(DMASEG, drivep->sector_size -
		2 - sizeof(struct elks_disk_parms));

	    /* first check for ELKS parm block */
	    if (parms->marker[0] == 'e' && parms->marker[1] == 'L'
		&& parms->size >= offsetof(struct elks_disk_parms, size)) {
		drivep->cylinders = parms->track_max;
		drivep->sectors = parms->sect_max;
		drivep->heads = parms->head_max;

		if (drivep->cylinders != 0 && drivep->sectors != 0
		    && drivep->heads != 0) {
		    found_PB = 1;
#if DEBUG_PROBE
		    printk("fd: found valid ELKS CHS %d,%d,%d disk parameters on /dev/fd%d "
			   "boot sector\n", drivep->cylinders, drivep->heads, drivep->sectors,
                           target);
#endif
		    goto got_geom;
		}
	    }

	    /* second check for valid FAT BIOS parm block */
	    unsigned char __far *boot = _MK_FP(DMASEG, 0);
	    if (
		//(boot[510] == 0x55 && boot[511] == 0xAA) &&	/* bootable sig*/
		((boot[3] == 'M' && boot[4] == 'S') ||		/* OEM 'MSDOS'*/
		 (boot[3] == 'I' && boot[4] == 'B'))	 &&	/* or 'IBM'*/
		(boot[54] == 'F' && boot[55] == 'A')	   ) {	/* v4.0 fil_sys 'FAT'*/

		/* has valid MSDOS 4.0+ FAT BPB, use it */
		drivep->sectors = boot[24];		/* bpb_sec_per_trk */
		drivep->heads = boot[26];		/* bpb_num_heads */
		unsigned char media = boot[21];		/* bpb_media_byte */
		drivep->cylinders =
			(media == 0xFD)? 40:
#ifdef CONFIG_IMG_FD1232
			(media == 0xFE)? 77:		/* FD1232 is 77 tracks */
#endif
					 80;
		drivep->cylinders = (media == 0xFD)? 40: 80;
		found_PB = 2;
#if DEBUG_PROBE
		    printk("fd: found valid FAT CHS %d,%d,%d disk parameters on /dev/fd%d "
			   "boot sector\n", drivep->cylinders, drivep->heads, drivep->cylinders,
                           target);
#endif
		goto got_geom;
	    }
	}
#if DEBUG_PROBE
        else {
            printk("fd: can't read boot sector\n");
        }
#endif
#endif /* FORCE_PROBE */

#if DEBUG_PROBE
	printk("fd: probing disc in /dev/fd%d\n", target);
#endif

	drivep->heads = 2;

/* First probe for cylinder number. We probe on sector 1, which is
 * safe for all formats, and if we get a seek error, we assume that
 * the previous format is the correct one.
 */

	count = 0;
#ifdef CONFIG_ARCH_PC98
	do {
	    if (count)
	        switch_device98(target, 0x30, drivep);	/* 1.44 MB */
	    /* skip probing first entry */
	    if (count && read_sector(target, track_probe[count] - 1, 1)) {
	        switch_device98(target, 0x90, drivep);	/* 1.232 MB */
	        break;
	    }
	    drivep->cylinders = track_probe[count];
	} while (++count < sizeof(track_probe)/sizeof(track_probe[0]));
#else
	do {
	    /* skip probing first entry */
	    if (count) {
		int res = read_sector(target, track_probe[count] - 1, 1);
#if DEBUG_PROBE
		printk("CYL %d %s, ", track_probe[count]-1, res? "fail": "ok");
#endif
		if (res)
		    break;
	    }
	    drivep->cylinders = track_probe[count];
	} while (++count < sizeof(track_probe)/sizeof(track_probe[0]));
#endif

/* Next, probe for sector number. We probe on track 0, which is
 * safe for all formats, and if we get a seek error, we assume that
 * the previous successfully probed format is the correct one, or if none,
 * use the BIOS disk parameters.
 */

	count = 0;
#ifdef CONFIG_ARCH_PC98
	do {
	    if (count)
	        switch_device98(target, 0x30, drivep);	/* 1.44 MB */
	    /* skip reading first entry */
	    if (count && read_sector(target, 0, sector_probe[count])) {
	        switch_device98(target, 0x90, drivep);	/* 1.232 MB */
	        break;
	    }
	    drivep->sectors = sector_probe[count];
	} while (++count < sizeof(sector_probe)/sizeof(sector_probe[0]));
#else
	do {
	    int res = read_sector(target, 0, sector_probe[count]);
#if DEBUG_PROBE
	    printk("SEC %d %s, ", sector_probe[count], res? "fail": "ok");
#endif
	    if (res) {
                if (count == 0) {	/* failed on first sector read, use BIOS parms */
		    printk("fd: disc probe failed, using BIOS settings\n");
		    *drivep = fd_types[drivep->fdtype];
		    goto got_geom;
		}
		break;
	    }
	    drivep->sectors = sector_probe[count];
	} while (++count < sizeof(sector_probe)/sizeof(sector_probe[0]));
#endif

#if DEBUG_PROBE
    printk("\n");
#endif

got_geom:
	printk("fd: /dev/fd%d %s has %d cylinders, %d heads, and %d sectors\n", target,
		   (found_PB == 2)? "DOS format," :
		   (found_PB == 1)? "ELKS bootable,": "probed, probably",
		   drivep->cylinders, drivep->heads, drivep->sectors);
	hdp->start_sect = 0;
	hdp->nr_sects = ((sector_t)(drivep->sectors * drivep->heads))
				* ((sector_t)drivep->cylinders);
    }
}
#endif /* CONFIG_BLK_DEV_BFD*/

static int bioshd_open(struct inode *inode, struct file *filp)
{
    unsigned int target = DEVICE_NR(inode->i_rdev);	/* >> MINOR_SHIFT */
    struct hd_struct *hdp = &hd[MINOR(inode->i_rdev)];

    if (!bioshd_initialized || target >= NUM_DRIVES || hdp->start_sect == -1U)
	return -ENXIO;

#if 0
    while (busy[target])
	sleep_on(&busy_wait);
#endif

    access_count[target]++;	/* Register that we're using the device */

#ifdef CONFIG_BLK_DEV_BFD
    if (access_count[target] == 1)	/* probe only on initial open*/
	probe_floppy(target, hdp);
#endif

    inode->i_size = hdp->nr_sects * drive_info[target].sector_size;
    /* limit inode size to max filesize for CHS >= 4MB (2^22)*/
    if (hdp->nr_sects >= 0x00400000L)	/* 2^22*/
	inode->i_size = 0x7ffffffL;	/* 2^31 - 1*/
    return 0;
}

static struct file_operations bioshd_fops = {
    NULL,			/* lseek - default */
    block_read,			/* read - general block-dev read */
    block_write,		/* write - general block-dev write */
    NULL,			/* readdir - bad */
    NULL,			/* select */
    bioshd_ioctl,		/* ioctl */
    bioshd_open,		/* open */
    bioshd_release		/* release */
};

int INITPROC bioshd_init(void)
{
    register struct gendisk *ptr;
    int count;

    /* FIXME perhaps remove for speed on floppy boot*/
    outb_p(0x0C, FDC_DOR);	/* FD motors off, enable IRQ and DMA*/

#ifdef CONFIG_BLK_DEV_BFD
    _fd_count = bioshd_getfdinfo();
#if NOTNEEDED
    enable_irq(FLOPPY_IRQ);	/* Floppy */
#endif
#endif
#ifdef CONFIG_BLK_DEV_BHD
    _hd_count = bioshd_gethdinfo();
#if NOTNEEDED
    if (sys_caps & CAP_PC_AT) {	/* PC/AT or greater */
	enable_irq(HD1_AT_IRQ);	/* AT ST506 */
	enable_irq(HD2_AT_IRQ);	/* AHA1542 */
    }
    else {
	enable_irq(HD_IRQ);	/* XT ST506 */
    }
#endif
    bioshd_gendisk.nr_real = _hd_count;
#endif /* CONFIG_BLK_DEV_BHD */

#ifdef PRINT_DRIVE_INFO
    {
	register struct drive_infot *drivep;
	static char UNITS[4] = "kMGT";

	drivep = drive_info;
	for (count = 0; count < PRINT_DRIVE_INFO; count++, drivep++) {
	    if (drivep->heads != 0) {
		char *unit = UNITS;
		__u32 size = ((__u32) drivep->sectors) * 5;	/* 0.1 kB units */
		if (drivep->sector_size == 1024)
		    size <<= 1;
		size *= ((__u32) drivep->cylinders) * drivep->heads;

		/* Select appropriate unit */
		while (size > 99999 && unit[1]) {
		    debug("DBG: Size = %lu (%X/%X)\n", size, *unit, unit[1]);
		    size += 512U;
		    size /= 1024U;
		    unit++;
		}
		debug("DBG: Size = %lu (%X/%X)\n",size,*unit,unit[1]);
		printk("/dev/%cd%c: %u cylinders, %d heads, %d sectors = %lu.%u %cb\n",
		    (count < 4 ? 'h' : 'f'), (count & 3) + (count < 4 ? 'a' : '0'),
		    drivep->cylinders, drivep->heads, drivep->sectors,
		    (size/10), (int) (size%10), *unit);
	    }
	}
    }
#else /* one line version */
#ifdef CONFIG_BLK_DEV_BFD
#ifdef CONFIG_BLK_DEV_BHD
    printk("bioshd: %d floppy drive%s and %d hard drive%s\n",
	   _fd_count, _fd_count == 1 ? "" : "s",
	   _hd_count, _hd_count == 1 ? "" : "s");
#else
    printk("bioshd: %d floppy drive%s\n",
	   _fd_count, _fd_count == 1 ? "" : "s");
#endif
#else
#ifdef CONFIG_BLK_DEV_BHD
    printk("bioshd: %d hard drive%s\n",
	   _hd_count, _hd_count == 1 ? "" : "s");
#endif
#endif
#endif /* PRINT_DRIVE_INFO */

    if (!(_fd_count + _hd_count)) return 0;

    copy_ddpt();	/* make a RAM copy of the disk drive parameter table*/

    count = register_blkdev(MAJOR_NR, DEVICE_NAME, &bioshd_fops);

    if (count == 0) {
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;

	if (gendisk_head == NULL) {
	    bioshd_gendisk.next = gendisk_head;
	    gendisk_head = &bioshd_gendisk;
	} else {
	    for (ptr = gendisk_head; ptr->next != NULL; ptr = ptr->next)
		/* Do nothing */ ;
	    ptr->next = &bioshd_gendisk;
	    bioshd_gendisk.next = NULL;
	}
	bioshd_initialized = 1;
    } else {
	printk("bioshd: unable to register %d\n", MAJOR_NR);
    }
    return count;
}

static int bioshd_ioctl(struct inode *inode,
			struct file *file, unsigned int cmd, unsigned int arg)
{
    register struct hd_geometry *loc = (struct hd_geometry *) arg;
    register struct drive_infot *drivep;
    int dev, err;

    /* get sector size called with NULL inode and arg = superblock s_dev */
    if (cmd == HDIO_GET_SECTOR_SIZE)
	return drive_info[DEVICE_NR(arg)].sector_size;

    if (!inode || !inode->i_rdev)
	return -EINVAL;

    dev = DEVICE_NR(inode->i_rdev);
    if (dev >= ((dev < DRIVE_FD0) ? _hd_count : (DRIVE_FD0 + _fd_count)))
    	return -ENODEV;

    drivep = &drive_info[dev];
    err = -EINVAL;
    switch (cmd) {
    case HDIO_GETGEO:
	err = verify_area(VERIFY_WRITE, (void *) arg, sizeof(struct hd_geometry));
	if (!err) {
	    put_user_char(drivep->heads, &loc->heads);
	    put_user_char(drivep->sectors, &loc->sectors);
	    put_user(drivep->cylinders, &loc->cylinders);
	    put_user_long(hd[MINOR(inode->i_rdev)].start_sect, &loc->start);
	}
    }
    return err;
}

/* calculate CHS and sectors remaining for track read */
static void get_chst(struct drive_infot *drivep, sector_t start, unsigned int *c,
	unsigned int *h, unsigned int *s, unsigned int *t)
{
	sector_t tmp;

	*s = (unsigned int) ((start % (sector_t)drivep->sectors) + 1);
	tmp = start / (sector_t)drivep->sectors;
	*h = (unsigned int) (tmp % (sector_t)drivep->heads);
	*c = (unsigned int) (tmp / (sector_t)drivep->heads);
	*t = drivep->sectors - *s + 1;
	debug_bios("bioshd: lba %ld is CHS %d/%d/%d remaining sectors %d\n",
		start, *c, *h, *s, *t);
}

/* do bios I/O, return # sectors read/written */
static int do_bios_readwrite(struct drive_infot *drivep, sector_t start, char *buf,
	ramdesc_t seg, int cmd, unsigned int count)
{
	int drive, errs;
	unsigned int cylinder, head, sector, this_pass;
	unsigned int segment, offset;
	unsigned short in_ax, out_ax;

	drive = drivep - drive_info;
	map_drive(&drive);
	get_chst(drivep, start, &cylinder, &head, &sector, &this_pass);

	/* limit I/O to requested sector count*/
	if (this_pass > count) this_pass = count;
	if (cmd == READ) debug_bios("bioshd(%d): read lba %ld count %d\n",
				drive, start, this_pass);

	errs = MAX_ERRS;	/* BIOS disk reads should be retried at least three times */
	do {
#ifdef CONFIG_FS_XMS_BUFFER
		if (seg >> 16) {
			segment = DMASEG;		/* if xms buffer use DMASEG*/
			offset = 0;
			if (cmd == WRITE)	/* copy xms buffer down before write*/
				xms_fmemcpyw(0, DMASEG, buf, seg, this_pass*(drivep->sector_size >> 1));
			set_cache_invalid();
		} else
#endif
		{
			segment = (seg_t)seg;
			offset = (unsigned) buf;
		}
		debug_bios("bioshd(%d): cmd %d CHS %d/%d/%d count %d\n",
		    drive, cmd, cylinder, head, sector, this_pass);
		in_ax = BD_AX;
		out_ax = 0;

		set_ddpt(drivep->sectors);
		if (bios_disk_rw(cmd == WRITE? BIOSHD_WRITE: BIOSHD_READ, this_pass,
					drive, cylinder, head, sector, segment, offset)) {
			printk("bioshd(%d): cmd %d retry #%d CHS %d/%d/%d count %d\n",
			    drive, cmd, MAX_ERRS - errs + 1, cylinder, head, sector, this_pass);
		    out_ax = BD_AX;
		    reset_bioshd(drive);
		}
	} while (out_ax && --errs);	/* On error, retry up to MAX_ERRS times */
	last_drive = drivep;

	if (out_ax) {
		printk("bioshd: error: out AX=%04X in AX=%04X "
		       "ES:BX=%04X:%04X\n", out_ax, in_ax, BD_ES, BD_BX);
		return 0;
	}
#ifdef CONFIG_FS_XMS_BUFFER
	if (seg >> 16) {
		if (cmd == READ)	/* copy DMASEG up to xms*/
			xms_fmemcpyw(buf, seg, 0, DMASEG, this_pass*(drivep->sector_size >> 1));
		set_cache_invalid();
	}
#endif
	return this_pass;
}

#ifdef CONFIG_TRACK_CACHE		/* use track-sized sector cache*/
static sector_t cache_startsector;
static sector_t cache_endsector;

/* read from start sector to end of track into DMASEG track buffer, no retries*/
static void bios_readtrack(struct drive_infot *drivep, sector_t start)
{
	unsigned int cylinder, head, sector, num_sectors;
	int drive = drivep - drive_info;
	int errs = 0;
	unsigned short out_ax;

	map_drive(&drive);
	get_chst(drivep, start, &cylinder, &head, &sector, &num_sectors);

	if (num_sectors > (DMASEGSZ / drivep->sector_size))
		num_sectors = DMASEGSZ / drivep->sector_size;

	do {
		out_ax = 0;
		debug_bios("bioshd(%d): track read CHS %d/%d/%d count %d\n",
			drive, cylinder, head, sector, num_sectors);

		set_ddpt(drivep->sectors);
		if (bios_disk_rw(BIOSHD_READ, num_sectors, drive,
						cylinder, head, sector, DMASEG, 0)) {
			printk("bioshd(%d): track read retry #%d CHS %d/%d/%d count %d\n",
			    drive, errs + 1, cylinder, head, sector, num_sectors);
		    out_ax = BD_AX;
		    reset_bioshd(drive);
		}
	} while (out_ax && ++errs < 1);	/* no track retries, for testing only*/
	last_drive = drivep;

	if (out_ax) {
		set_cache_invalid();
		return;
	}

	cache_drive = drivep;
	cache_startsector = start;
	cache_endsector = start + num_sectors - 1;
	debug_bios("bioshd(%d): track read lba %ld to %ld count %d\n",
		drive, cache_startsector, cache_endsector, num_sectors);
}

/* check whether cache is valid for one sector*/
static int cache_valid(struct drive_infot *drivep, sector_t start, char *buf,
	ramdesc_t seg)
{
	unsigned int offset;

	if (drivep != cache_drive || start < cache_startsector || start > cache_endsector)
	    return 0;

	offset = (int)(start - cache_startsector) * drivep->sector_size;
	debug_bios("bioshd(%d): cache hit lba %ld\n", hd_drive_map[drivep-drive_info], start);
	xms_fmemcpyw(buf, seg, (void *)offset, DMASEG, drivep->sector_size >> 1);
	return 1;
}

/* read from cache, return # sectors read*/
static int do_cache_read(struct drive_infot *drivep, sector_t start, char *buf,
	ramdesc_t seg, int cmd)
{
	if (cmd == READ) {
	    if (cache_valid(drivep, start, buf, seg))	/* try cache first*/
		return 1;
	    bios_readtrack(drivep, start);		/* read whole track*/
	    if (cache_valid(drivep, start, buf, seg)) 	/* try cache again*/
		return 1;
	}
	set_cache_invalid();
	return 0;
}
#endif

static void do_bioshd_request(void)
{
	struct drive_infot *drivep;
	struct request *req;
	unsigned short minor;
	sector_t start;
	int drive, count;
	char *buf;

	spin_timer(1);
	while (1) {
      next_block:

	req = CURRENT;
	if (!req)	/* break for spin_timer stop before INIT_REQUEST */
	    break;

	INIT_REQUEST(req);

	if (bioshd_initialized != 1) {
	    end_request(0);
	    continue;
	}
	minor = MINOR(req->rq_dev);
	drive = minor >> MINOR_SHIFT;
	drivep = &drive_info[drive];

	/* make sure it's a disk that we are dealing with. */
	if (drive > (DRIVE_FD0 + MAXDRIVES - 1) || drivep->heads == 0) {
	    printk("bioshd: non-existent drive\n");
	    end_request(0);
	    continue;
	}

	/* all ELKS requests are 1K blocks*/
	count = BLOCK_SIZE / drivep->sector_size;
	start = req->rq_blocknr * count;

	if (hd[minor].start_sect == -1U || start >= hd[minor].nr_sects) {
	    printk("bioshd: bad partition start=%ld sect=%ld nr_sects=%ld.\n",
		   start, hd[minor].start_sect, hd[minor].nr_sects);
	    end_request(0);
	    continue;
	}
	start += hd[minor].start_sect;

	buf = req->rq_buffer;
	while (count > 0) {
	    int num_sectors;
#ifdef CONFIG_TRACK_CACHE
	    /* first try reading track cache*/
	    num_sectors = do_cache_read(drivep, start, buf, req->rq_seg, req->rq_cmd);
	    if (!num_sectors)
#endif
		/* then fallback with retries if required*/
		num_sectors = do_bios_readwrite(drivep, start, buf, req->rq_seg, req->rq_cmd,
			count);

	    if (num_sectors == 0) {
		end_request(0);
		goto next_block;
	    }

	    count -= num_sectors;
	    start += num_sectors;
	    buf += num_sectors * drivep->sector_size;
	}

	/* satisfied that request */
	end_request(1);
    }
    spin_timer(0);
}

#if 0			/* Currently not used, removing for size. */
static struct wait_queue busy_wait;
static int revalidate_hddisk(int, int);	/* Currently not used*/

#define DEVICE_BUSY busy[target]
#define USAGE access_count[target]
#define CAPACITY ((sector_t)drive_info[target].heads*drive_info[target].sectors*drive_info[target].cylinders)

/* We assume that the the bios parameters do not change,
 * so the disk capacity will not change
 */

#undef MAYBE_REINIT
#define GENDISK_STRUCT bioshd_gendisk

/* This routine is called to flush all partitions and partition tables
 * for a changed cdrom drive, and then re-read the new partition table.
 * If we are revalidating a disk because of a media change, then we
 * enter with usage == 0.  If we are using an ioctl, we automatically
 * have usage == 1 (we need an open channel to use an ioctl :-), so
 * this is our limit.
 */

#ifndef MAYBE_REINIT
#define MAYBE_REINIT
#endif

static int revalidate_hddisk(int dev, int maxusage)
{
    register struct gendisk *gdev;
    int i, major, max_p, start, target;

    target = DEVICE_NR(dev);
    gdev = &GENDISK_STRUCT;
    clr_irq();
    if (DEVICE_BUSY || USAGE > maxusage) {
	set_irq();
	return -EBUSY;
    }
    DEVICE_BUSY = 1;
    set_irq();
    max_p = gdev->max_p;
    start = target << gdev->minor_shift;
    major = MAJOR_NR << 8;
    for (i = max_p - 1; i >= 0; i--) {
	sync_dev(major | start | i);
	invalidate_inodes(major | start | i);
	invalidate_buffers(major | start | i);
	gdev->part[start + i].start_sect = 0;
	gdev->part[start + i].nr_sects = 0;
    };
    MAYBE_REINIT;
    gdev->part[start].nr_sects = CAPACITY;
    resetup_one_dev(gdev, target);
    DEVICE_BUSY = 0;
    wake_up(&busy_wait);
    return 0;
}
#endif

static void bioshd_geninit(void)
{
    register struct drive_infot *drivep;
    register struct hd_struct *hdp = hd;
    int i;

    drivep = drive_info;
    for (i = 0; i < NUM_DRIVES << MINOR_SHIFT; i++) {
	if ((i & ((1 << MINOR_SHIFT) - 1)) == 0) {
	    hdp->nr_sects = (sector_t) drivep->sectors *
		drivep->heads * drivep->cylinders;
	    hdp->start_sect = 0;
	    drivep++;
	} else {
	    hdp->nr_sects = 0;
	    hdp->start_sect = -1;
	}
	hdp++;
    }

#if 0
    blksize_size[MAJOR_NR] = 1024;	/* Currently unused */
#endif

}

/* convert a bios drive number to a bioshd kdev_t*/
kdev_t INITPROC bioshd_conv_bios_drive(unsigned int biosdrive)
{
    int minor;
    int partition = 0;
    extern int boot_partition;

#ifdef CONFIG_ARCH_PC98
    if (((biosdrive & 0xF0) == 0x80) || ((biosdrive & 0xF0) == 0xA0)) {		/* hard drive*/
	for (minor = 0; minor < 4; minor++) {
	    if (biosdrive == hd_drive_map[minor]) break;
	}
	if (minor >= 4) minor = 0;
	partition = boot_partition;	/* saved from add_partition()*/
    } else {
	for (minor = 4; minor < 8; minor++) {
	    if (biosdrive == hd_drive_map[minor]) break;
	}
	if (minor >= 8) minor = 4;
    }
#else
    if (biosdrive & 0x80) {		/* hard drive*/
	minor = biosdrive & 0x03;
	partition = boot_partition;	/* saved from add_partition()*/
    } else
	minor = (biosdrive & 0x03) + DRIVE_FD0;
#endif

    return MKDEV(BIOSHD_MAJOR, (minor << MINOR_SHIFT) + partition);
}
