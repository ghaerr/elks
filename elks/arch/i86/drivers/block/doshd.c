/* doshd.c */
/* Copyright (C) 1994 Yggdrasil Computing, Incorporated
   4880 Stevens Creek Blvd. Suite 205
   San Jose, CA 95129-1034
   USA
   Tel (408) 261-6630
   Fax (408) 261-6631

   This file is part of the Linux Kernel

   Linux is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Linux is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Linux; see the file COPYING.  If not, write to
   the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
/*#include <linux/head.h> */
#include <linuxmt/genhd.h>
#include <linuxmt/hdreg.h>
#include <linuxmt/biosparm.h>
#include <linuxmt/major.h>
#include <linuxmt/bioshd.h>
#include <linuxmt/fs.h>
/*#include <linux/signal.h> */
#include <linuxmt/string.h>
#include <linuxmt/mm.h>
#include <linuxmt/config.h>
#include <arch/segment.h>
#include <arch/system.h>

#define MAJOR_NR BIOSHD_MAJOR
#define BIOSDISK
#define NR_HD ((drive_info[1].heads)?2:((drive_info[0].heads)?1:0))
#define put_user(val,ptr) pokew(current->t_regs.ds,ptr,val)
#include "blk.h"

/* This doesn't need to be defined if user deselects BIOS FD code */

#ifdef CONFIG_BLK_DEV_BFD
/* #define USE_OLD_PROBE */
#endif

/* Uncomment this if your floppy drive(s) can't be properly recognized */
/* Search for HARDCODED and adjust ndrives and drive_info[] to match your */
/* system */

/* #define HARDCODED */

/* Uncomment this if the driver needs to support requests that are not
 * 1024 bytes (2 sectors)
 */

/* #define MULT_SECT_RQ */

#define BUFSEG 0x800

#ifdef CONFIG_BLK_DEV_BIOS
static int bioshd_ioctl();
static int bioshd_open();
static void bioshd_release();

static struct file_operations bioshd_fops =
{
	NULL,			/* lseek - default */
	block_read,		/* read - general block-dev read */
	block_write,		/* write - general block-dev write */
	NULL,			/* readdir - bad */
	NULL,			/* select */
	bioshd_ioctl,		/* ioctl */
	bioshd_open,		/* open */
	bioshd_release,		/* release */
#ifdef BLOAT_FS
	NULL,			/* fsync */
	NULL,			/* check_media_change */
	NULL,			/* revalidate */
#endif
};

/* static struct wait_queue *busy_wait = NULL; */
static struct wait_queue dma_wait;
static int dma_avail = 1;

static int bioshd_initialized = 0;
/* static int force_bioshd; */
static int revalidate_hddisk();
static struct drive_infot
{
	int cylinders;
	int sectors;
	int heads;
	int fdtype; /* matches fd_types or -1 if hd */
}
drive_info[4];

struct drive_infot fd_types[] = {
	{40, 9, 2, 0},
	{80, 15, 2, 1},
	{80, 9, 2, 2},
	{80, 18, 2, 3},
	{80, 36, 2, 4},
};

/* This makes probing order more logical */
/* and avoids few senseless seeks in some cases */
static int probe_order[5] = {0, 2, 1, 3, 4};

static struct hd_struct hd[4 << 6];
/* static char busy[4] = {0,}; */
static int access_count[4] = {0,};
static unsigned char hd_drive_map[4] =
{
	0x80, 0x81,		/* hda, hdb */
	0x00, 0x01		/* fda, fdb */
};



static int bioshd_sizes[4 << 6] = {0,};

static void bioshd_geninit();

static struct gendisk bioshd_gendisk =
{
	MAJOR_NR,		/* Major number */
	"bd",			/* Major name */
	6,			/* Bits to shift to get real from partition */
	1 << 6,			/* Number of partitions per real */
	4,			/* maximum number of real */
	bioshd_geninit,		/* init function */
	hd,			/* hd struct */
	bioshd_sizes,		/* sizes not blocksizes */
	0,			/* number */
	(void *) drive_info,	/* internal */
	NULL			/* next */
};

/* This function checks to see which hard drives are active and sets up the
 * drive_info[] table for them.  Ack this is darned confusing... */

#ifdef CONFIG_BLK_DEV_BHD
int bioshd_gethdinfo()
{
	int drive, ndrives = 0;

	for (drive = 0; drive <= 1; drive++) {
		register  struct drive_infot * drivep = &drive_info[drive];
		ndrives++;
		BD_AX = BIOSHD_DRIVE_PARMS;
		BD_DX = drive + 0x80; 
		BD_IRQ = BIOSHD_INT;
		call_bios();
		if ((BD_AX != 0x100) && (!CARRY_SET)) {
			drivep->cylinders = ((BD_CX >> 8) & 255);
			drivep->cylinders += 
					(((BD_CX >> 6) & 3) * 256); 
			drivep->heads = ((BD_DX >> 8) & 63) + 1;
			drivep->sectors = (BD_CX & 63);
			drivep->fdtype = -1;
		}
		if ((BD_DX & 255) < 2) break;
	}
	return ndrives;
}
#endif
#ifdef CONFIG_BLK_DEV_BFD
int bioshd_getfdinfo()
{
#ifdef HARDCODED
	/* Set this to match your system */
	/* Currently it's set to two drive system, 1.44MB as /dev/fd0 */
	/* and 1.2 MB as /dev/fd1 */

	/* ndrives is number of drives in your system (either 0, 1 or 2) */

	int ndrives = 2;

	/* drive_info[] should be set *only* for existing drives; */
	/* comment out drive_info lines if you don't need them (e.g.*/
	/* you have less than 2 drives) */

	/* Enter type 4 in fd_types' brackets for unknown drive type */
	/* Otherwise use table below */

	/* Floppy drive type table:
		Type:	Format:
		0	360 KB
		1	1.2 MB
		2	720 KB
		3	1.44 MB
		4	2.88 MB/Unknown
	*/

	/* /dev/fd0 */
	drive_info[2] = fd_types[2];

	/* /dev/fd1 */
	drive_info[3] = fd_types[2];
/*
	if (drive_info[2].fdtype != 0) 
		printk("drive 0 is %d\n", drive_info[2].fdtype); 
	if (drive_info[3].fdtype != 0) 
		printk("drive 1 is %d\n", drive_info[3].fdtype); 
*/
	/* That's it .. you're done :-) */

	/* Warning: drive will be reported as 2880 KB at bootup if you've */
	/* set it as unknown (4). Floppy probe will detect correct floppy */
	/* format at each change so don't bother with that */

#else

	int drive, ndrives;

	/* We get the # of drives from the BPB, which is PC-friendly */
#ifdef ROM_GETFLOPPY_VIA_INT13
        BD_AX = BIOSHD_DRIVE_PARMS;
        BD_DX = 0;             /* only the number floppies */
        BD_IRQ = BIOSHD_INT;
        call_bios();
        ndrives = (CARRY_SET)?0:BD_DX & 0xff;
#else        
	ndrives = (peekb(0x40, 0x10) >> 6) + 1;
#endif
	printk("doshd: found %d floppy drives\n", ndrives);

	for (drive = 0; drive < ndrives; drive++) {
		BD_AX = BIOSHD_DRIVE_PARMS;
		BD_DX = drive;
		BD_BX = 0;
		BD_IRQ = BIOSHD_INT;
/*		printk("%x\n", BD_AX); */
/*		call_bios(); */
/*		printk("%x\n", BD_AX); */
#ifdef ROM_GETFLOPPY_VIA_INT13
                call_bios();
                if ((!CARRY_SET) && ((BD_AX & 0xff00) == 0)) {
		   drive_info[drive+2] = fd_types[BD_BX - 1];
		}
		else printk("error in drivetype %d\n",drive);
#else
		if ((arch_cpu > 5) && (call_bios(),(!CARRY_SET)) && (BD_AX != 0x100)) { 
			/* Some XT's return strange results - Al */
			/* The arch_cpu is a safety check */
			/* AT archecture, drive type in DX */
			/* Set to type 3 as this is less drastic */
/*			printk("drive %d is %d\n", drive, BD_BX); */
			drive_info[drive+2] = fd_types[BD_BX - 1];
		} else {	
			/* Cannot be determined correctly */
			/* Type 4 should work on all systems */
/*			printk("drive %d type unknown\n", drive); */
			drive_info[drive+2] = fd_types[4];
		}
#endif		
	}
#endif /* HARDCODED */

	return ndrives;
}
#endif /* CONFIG_BLK_DEV_BFD */

static void bioshd_release(inode, filp)
struct inode *inode;
struct file *filp;
{
	int target;
	kdev_t	dev = inode->i_rdev;
	sync_dev(dev);

	target = DEVICE_NR(dev);
	access_count[target]--;
	if ((target >= 2) && (access_count[target] == 0)) {
		invalidate_inodes(dev);
		invalidate_buffers(dev);
	}
	return;
}

/* As far as I can tell this doesn't actually
   work, but we might as well try it. 
   -- Some XT controllers are happy with it.. [AC]
 */

static void reset_bioshd(minor)
    int minor;
{
	BD_IRQ = BIOSHD_INT;
	BD_AX = BIOSHD_RESET;
	BD_DX = hd_drive_map[DEVICE_NR(minor)];

	call_bios();
	if (CARRY_SET || (BD_AX & 0xff00) != 0)
		printk("hd: unable to reset.\n");
	return;
}

#ifndef USE_OLD_PROBE /* we need this as we don't want to duplicate the code */
int seek_sector(drive, track, sector)
int drive, track, sector;
{
/* i took this code from bioshd_open() */
/* it replicates code there when new floppy probe is used */

	int count;

	for (count = 0; count < MAX_ERRS; count++)
	{
	/* BIOS read sector */
	
		BD_IRQ = BIOSHD_INT;
		BD_AX = BIOSHD_READ | 1;/* Read 1 sector */
		BD_BX = 0;		/* Seg offset = 0 */
		BD_ES = BUFSEG;		/* Segment to read to */
		BD_CX = ((track - 40) << 8) | sector;
		BD_DX = (0 << 8) | drive;
		BD_FL = 0;

		isti();
		call_bios();
		
		if (CARRY_SET)
			{
			if (((BD_AX >> 8) == 0x04) && (count == MAX_ERRS - 1)) 
				break; /* Sector not found */
				
			reset_bioshd(drive);
			}
			else 
				return 0; /* everything is OK */
		}
		
		return 1; /* error */

}
#endif

static int bioshd_open(inode, filp)
struct inode *inode;
struct file *filp;
{
	int target;
	int fdtype;
	register struct drive_infot * drivep;

	target = DEVICE_NR(inode->i_rdev);  /* >> 6 */
	drivep = &drive_info[target];
	fdtype = drivep->fdtype;

	/* Bounds testing */

	if (bioshd_initialized == 0)
		return (-ENXIO);
	if (target >= 4)
		return (-ENXIO);
	if (hd[MINOR(inode->i_rdev)].start_sect == -1)
		return (-ENXIO);

	/* Wait until it's free */

/*	while (busy[target])
		sleep_on(&busy_wait); */

	/* Register that we're using the device */

	access_count[target]++;

	/* Check for disk type */
	
	/* I guess it works now as it should. Tested under dosemu with 720Kb,
	1.2 MB and 1.44 MB floppy image and works fine - Blaz Antonic */

#ifdef CONFIG_BLK_DEV_BFD
	if (target >= 2) {         /* 2,3 are the floppydrives */
		int count;
		int i;
#endif
#ifndef USE_OLD_PROBE /* we need those for new probe */
		/* probing range can be easily extended by adding 
		more values to those two lists and adjusting for loop'
		parameters in line 420 and 435 (or somewhere near) */
		
		static char sector_probe[5] = {8, 9, 15, 18, 36};
		static char track_probe[2] = {40, 80};
#endif /* USE_OLD_PROBE */
#ifdef CONFIG_BLK_DEV_BFD

		printk("hd: probing disc in /dev/fd%d\n", target % 2);

		/* The area between 32-64K is a 'scratch' area - we
		 * need a semaphore for it */

		while (!dma_avail) sleep_on(&dma_wait);
		dma_avail = 0;
#endif
#ifdef USE_OLD_PROBE
		count = 0;
		for (i = 0; i <= probe_order[fdtype]; i++)
		{
			count = 0;
 
			while (count < MAX_ERRS)
			{
/*				printk("type %d count %d\n", probe_order[i], count); */
				/* BIOS read sector */

				BD_IRQ = BIOSHD_INT;
				BD_AX = BIOSHD_READ | 1;/* Read 1 sector */
				BD_BX = 0;		/* Seg offset = 0 */
				BD_ES = BUFSEG;		/* Segment to read to */
				BD_CX = ((fd_types[probe_order[i]].cylinders-40)<<8)|fd_types[probe_order[i]].sectors;
				BD_DX = (0 << 8) | hd_drive_map[target];	/* Head 0, drive number */
				BD_FL = 0;

				isti();
				call_bios();

				if (CARRY_SET)
				{
					if (((BD_AX >> 8) == 0x04) && (count == MAX_ERRS - 1)) 
						break; /* Sector not found */

					reset_bioshd(hd_drive_map[target]);
					count++;
				} else {
					drivep->cylinders = fd_types[probe_order[i]].cylinders;
					drivep->sectors = fd_types[probe_order[i]].sectors;
					break;
				}   
			}
		}

#else /* USE_OLD_PROBE */
		/* first probe for track number */
#ifndef GET_DISKPARAM_BY_INT13_NO_SEEK
		for (count = 0; count < 2; count++)
		{
		/* we probe on sector 1, which is safe for all formats */
		if (seek_sector(hd_drive_map[target], track_probe[count], 1))
			/* seek error, assume type used before */
			{
			drivep->cylinders = track_probe[count - 1];
			break;
			}
		drivep->cylinders = track_probe[count];		
		}

		/* and then for sector number */		
		for (count = 0; count < 5; count++)
		{
		/* we probe on track 0 (40 - 40 in seek_sector), which is safe for all formats */
		if (seek_sector(hd_drive_map[target], 40, sector_probe[count]))
			/* seek error, assume type used before */
			{
			drivep->sectors = sector_probe[count - 1];
			break;
			}
		drivep->sectors = sector_probe[count];
		}
#else
/* We can get the Geometry of the floppy by bios */
		BD_IRQ = BIOSHD_INT;
		BD_AX = BIOSHD_DRIVE_PARMS;
		BD_DX = hd_drive_map[target];	/* Head 0, drive number */

		call_bios();
      if (!CARRY_SET) {
         drivep->sectors = (BD_CX & 0xff) +1;
         drivep->cylinders = (BD_CX >> 8) +1;
      }
      else printk("bioshd_open: no diskinfo %d\n",hd_drive_map[target]);
#endif

#endif /* USE_OLD_PROBE */
#ifdef CONFIG_BLK_DEV_BFD
		/* DMA code belongs out of the loop. */
		dma_avail = 1;
		wake_up(&dma_wait);

		/* I moved this out of for loop to prevent trashing the screen
		with seducing (repeating) probe messages about disk types */

		if (drivep->cylinders == 0 || drivep->sectors == 0)
			printk("hd: Autoprobe failed!\n");
		else
			printk("hd: disc in /dev/fd%d probably has %d sectors and %d cylinders\n", target % 2, drivep->sectors, drivep->cylinders);

/*
 *	This is not a bugfix, hence no code, but coders should be aware
 *	that multi-sector reads from this point on depend on bootsect
 *	modifying the default Disk Parameter Block in BIOS.
 *	dpb[4] should be set to a high value such as 36 so that reads
 *	can go past what is hardwired in the BIOS.
 *	36 is the number of sectors in a 2.88 floppy track.
 *	If you are booting ELKS with anything other than bootsect you
 *	have to make equivalent arrangements.
 *	0:0x78 contains address of dpb (char dpb[12]), and dpb[4] is the End of
 *	Track parameter for the 765 Floppy Disk Controller.
 *	You may have to copy dpb to RAM as the original is in ROM.
 */
	}
#endif
	return 0;
}

void init_bioshd()
{
	int i;
	int count = 0;
#ifdef CONFIG_BLK_DEV_BHD
	int hdcount = 0;
#endif
	register struct gendisk *ptr;
	register struct drive_infot * drivep;
	int addr = 0x8c;

	printk("hd Driver Copyright (C) 1994 Yggdrasil Computing, Inc.\nExtended and modified for Linux8086 by Alan Cox.\n");

#ifdef CONFIG_BLK_DEV_BFD
	bioshd_getfdinfo();
#endif
#ifdef CONFIG_BLK_DEV_BHD
	bioshd_gethdinfo();
#endif

	for (i = 0; i <= 3; i++) {
		if (drive_info[i].heads) {
			count++;
#ifdef CONFIG_BLK_DEV_BHD
			if (i <= 1)
				hdcount++;
#endif
		} 
	}

	if (!count)
		return;

#ifdef CONFIG_BLK_DEV_BHD
	printk("doshd: found %d hard drive%c\n", hdcount, hdcount == 1 ? ' ' : 's');
#endif

#ifdef DOSHD_VERBOSE_DRIVES
	for (i = 0; i < 4; i++)
	{
		drivep = &drive_info[i];
		if (drivep->heads != 0)
		{
			printk("/dev/%cd%c: %d heads, %d cylinders, %d sectors = %ld %s\n",
				(i < 2 ? 'h' : 'f'), (i % 2) + (i < 2 ? 'a' : '0'),
				drivep->heads,
				drivep->cylinders,
				drivep->sectors,
				(sector_t) drivep->heads * drivep->cylinders *
				drivep->sectors * 512L / (1024L * (i < 2 ? 1024L : 1L)),
				(i < 2 ? "MB" : "kB"));
		}
	}
#else /* DOSHD_VERBOSE_DRIVES */
#ifdef CONFIG_BLK_DEV_BHD
	for (i=0; i < 2; i++)
	{
		drivep = &drive_info[i];
		if (drivep->heads != 0)
		{
			printk("/dev/bd%c: %d heads, %d cylinders, %d sectors = %ld MB\n",
				(i + 'a'),
				drivep->heads,
				drivep->cylinders,
				drivep->sectors,
				(sector_t) drivep->heads * drivep->cylinders *
				drivep->sectors * 512L / (1024L * (i < 2 ? 1024L : 1L)));
			}
	}
	bioshd_gendisk.nr_real = hdcount;
#endif /* CONFIG_BLK_DEV_BHD */
#endif /* DOSHD_VERBOSE_DRIVES */
			
	i = register_blkdev(MAJOR_NR, DEVICE_NAME, &bioshd_fops);

	if (i == 0) {
		blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
		/* blksize_size[MAJOR_NR] = 1024; */ /* Currently unused */
		/* read_ahead[MAJOR_NR] = 2; */ /* Currently unused */
		if (gendisk_head == NULL) {
			bioshd_gendisk.next = gendisk_head;
			gendisk_head = &bioshd_gendisk;
		} else {
			for (ptr = gendisk_head; ptr->next != NULL; ptr = ptr->next);
			ptr->next = &bioshd_gendisk;
			bioshd_gendisk.next = NULL;
		}
		bioshd_initialized = 1;
	} else
		printk("hd: unable to register\n");
}

static int bioshd_ioctl(inode, file, cmd, arg)
    struct inode *inode;
    struct file *file;
    unsigned int cmd;
    unsigned int arg;
{
	register struct hd_geometry *loc = (struct hd_geometry *) arg; /**/
	register struct drive_infot * drivep;
	int dev, err;

	if ((!inode) || !(inode->i_rdev))
		return -EINVAL;
	dev = DEVICE_NR(inode->i_rdev);
	drivep = &drive_info[dev];
	if (dev >= NR_HD)
		return -ENODEV;

	switch(cmd) {
		case HDIO_GETGEO:
			err = verify_area(VERIFY_WRITE,arg,sizeof(*loc));
			if (err)
				return err;
			put_user(drivep->heads, (char *) &loc->heads);
			put_user(drivep->sectors, (char *) &loc->sectors);
			put_user(drivep->cylinders, (short *) &loc->cylinders);
			put_user(hd[MINOR(inode->i_rdev)].start_sect, (sector_t *) &loc->start);

			return 0;
			break;
	}
		
	return -EINVAL;
}


static void do_bioshd_request()
{
	sector_t count;
	sector_t this_pass;
	register struct request * req;
	short cylinder;
	short head;
	short sector;
	sector_t start;
	int tmp;
	char *buff;
	int minor;
	int errs;
/*	int part; */
	int drive;

	while (1)
	{
	      next_block:

		/* make sure we have a valid request *//*Done by INIT_REQUEST*/
		if (!CURRENT || CURRENT->rq_dev < 0)
			return;

		/* now initialize it */
		INIT_REQUEST;

		/* make sure it's still valid */
		req = CURRENT;
		if (req == NULL || req->rq_sector == -1)
			return;

		if (bioshd_initialized != 1)
		{
			end_request(0);
			continue;
		}
		minor = MINOR(req->rq_dev);
/*		part = minor & ((1 << 6) - 1); */
		drive = minor >> 6;

		/* make sure it's a disk we are dealing with. */
		if (drive > 3 || drive < 0 || drive_info[drive].heads == -0)
		{
			printk("hd: non-existant drive\n");
			end_request(0);
			continue;
		}
#ifdef MULT_SECT_RQ
		count = req->rq_nr_sectors;
#else
		count = 2;
#endif
		start = req->rq_sector;
		buff = req->rq_buffer;

		if (hd[minor].start_sect == -1 || start >= hd[minor].nr_sects)
		{
			printk("hd: bad partition start=%d sect=%d nr_sects=%d.\n",
			       start, (int) hd[minor].start_sect, (int) hd[minor].nr_sects);
			end_request(0);
			continue;
		}
		start += hd[minor].start_sect;
		errs = 0;

		while (count > 0)
		{
			register struct drive_infot * drivep = &drive_info[drive];
			sector = (start % drivep->sectors) + 1;
			tmp = start / drivep->sectors;
			head = tmp % drivep->heads;
			cylinder = tmp / drivep->heads;

			this_pass = count;
			if (count <= (drivep->sectors - sector + 1))
				this_pass = count;
			else
				this_pass = drivep->sectors - sector + 1;

			while (!dma_avail) sleep_on(&dma_wait);
			dma_avail = 0;

			BD_IRQ = BIOSHD_INT;
			if (req->rq_cmd == WRITE) {
			   BD_AX = BIOSHD_WRITE | this_pass;
			   fmemcpy(BUFSEG, 0, get_ds(), buff, (this_pass * 512));
			}
			else {
				BD_AX = BIOSHD_READ | this_pass;
			}
			
			BD_BX = 0;
			BD_ES = BUFSEG;
			BD_CX = (cylinder << 8) | ((cylinder >> 2) & 0xc0) | sector;
			BD_DX = (head << 8) | hd_drive_map[drive];
			BD_FL = 0;
#if 0
			printk("cylinder=%d head=%d sector=%d drive=%d CMD=%d\n",
			    cylinder, head, sector, drive, req->cmd);
			printk("blocks %d\n", this_pass);
#endif

			isti();
			call_bios();

			if (CARRY_SET)
			{
				reset_bioshd(MINOR(req->rq_dev));
				dma_avail = 1;
				errs++;
				if (errs > MAX_ERRS)
				{
					printk("hd: error: AX=0x%x\n", BD_AX);
					end_request(0);
					wake_up(&dma_wait);
					goto next_block;
				}
				continue;	/* try again */
			}
			if (req->rq_cmd==READ) {
			  fmemcpy(get_ds(), buff, BUFSEG, 0, (this_pass * 512)); 
			}
			/* In case it's already been freed */	
			if (!dma_avail) {
				dma_avail = 1;
				wake_up(&dma_wait);
			}
			count -= this_pass;
			start += this_pass;
			buff += this_pass * 512;
		}
		/* satisfied that request */
		end_request(1);
	}
}

/* #define DEVICE_BUSY busy[target] */
#define USAGE access_count[target]
#define CAPACITY ((sector_t)drive_info[target].heads*drive_info[target].sectors*drive_info[target].cylinders)
/* We assume that the the bios parameters do not change, so the disk capacity
   will not change */
#undef MAYBE_REINIT
#define GENDISK_STRUCT bioshd_gendisk

/*
 * This routine is called to flush all partitions and partition tables
 * for a changed cdrom drive, and then re-read the new partition table.
 * If we are revalidating a disk because of a media change, then we
 * enter with usage == 0.  If we are using an ioctl, we automatically have
 * usage == 1 (we need an open channel to use an ioctl :-), so this
 * is our limit.
 */
#if 0 /* Currently not used, removing for size. */
static int revalidate_hddisk(dev, maxusage)
    int dev;
    int maxusage;
{
	int target, major;
	register struct gendisk *gdev;
	int max_p;
	int start;
	int i;

	target = DEVICE_NR(dev);
	gdev = &GENDISK_STRUCT;

	icli();
	if (DEVICE_BUSY || USAGE > maxusage)
	{
		isti();
		return -EBUSY;
	};
	DEVICE_BUSY = 1;
	isti();

	max_p = gdev->max_p;
	start = target << gdev->minor_shift;
	major = MAJOR_NR << 8;

	for (i = max_p - 1; i >= 0; i--)
	{
		sync_dev(major | start | i);
		invalidate_inodes(major | start | i);
		invalidate_buffers(major | start | i);
		gdev->part[start + i].start_sect = 0;
		gdev->part[start + i].nr_sects = 0;
	};

#ifdef MAYBE_REINIT
	MAYBE_REINIT;
#endif

	gdev->part[start].nr_sects = CAPACITY;
	resetup_one_dev(gdev, target);

	DEVICE_BUSY = 0;
	wake_up(&busy_wait);
	return 0;
}
#endif /* 0 */

static void bioshd_geninit()
{
	int i;
	register struct drive_infot * drivep;
	register struct hd_struct * hdp;

	for (i = 0; i < 4 << 6; i++)
	{
		drivep = &drive_info[i >> 6];
		hdp = &hd[i];
		if ((i & ((1 << 6) - 1)) == 0)
		{
			hdp->start_sect = 0;
			hdp->nr_sects = (sector_t) drivep->sectors *
			  drivep->heads *
			  drivep->cylinders;
		} else
		{
			hdp->start_sect = -1;
			hdp->nr_sects = 0;
		}
	}
	/* blksize_size[MAJOR_NR] = 1024; */ /* Currently unused */
}
#endif


