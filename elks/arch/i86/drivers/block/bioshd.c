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

#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/genhd.h>
#include <linuxmt/biosparm.h>
#include <linuxmt/major.h>
#include <linuxmt/fs.h>
#include <linuxmt/string.h>
#include <linuxmt/mm.h>
#include <linuxmt/memory.h>
#include <linuxmt/debug.h>
#include <linuxmt/timer.h>

#include <arch/hdreg.h>
#include <arch/io.h>
#include <arch/segment.h>
#include <arch/system.h>
#include <arch/irq.h>
#include <arch/ports.h>
#include "bioshd.h"

#define PER_DRIVE_INFO  1       /* =1 for per-line display of drive info at init */
#define DEBUG_PROBE     0       /* =1 to display more floppy probing information */
#define FORCE_PROBE     0       /* =1 to force floppy probing */
#define FULL_TRACK      0       /* =1 to read full tracks when track caching */
//#define IODELAY       5       /* times 10ms, emulated delay for floppy on QEMU */
#define MAX_ERRS        5       /* maximum sector read/write error retries */

#define MAJOR_NR        BIOSHD_MAJOR
#include "blk.h"

struct elks_disk_parms {
    __u16 track_max;            /* number of tracks, little-endian */
    __u8 sect_max;              /* number of sectors per track */
    __u8 head_max;              /* number of disk sides/heads */
    __u8 size;                  /* size of parameter block (everything before
                                   this point) */
    __u8 marker[2];             /* should be "eL" */
} __attribute__((packed));

static int bioshd_initialized = 0;
static int fd_count = 0;                /* number of floppy disks */
static int hd_count = 0;                /* number of hard disks */

static int access_count[NUM_DRIVES];    /* device open count */
static struct drive_infot drive_info[NUM_DRIVES];   /* operating drive info */
static struct drive_infot *cache_drive;
struct drive_infot *last_drive;         /* set to last drivep-> used in read/write */
extern struct drive_infot fd_types[];   /* BIOS floppy formats */

static struct hd_struct hd[NUM_DRIVES << MINOR_SHIFT];  /* partitions start, size*/

static int bioshd_open(struct inode *, struct file *);
static void bioshd_release(struct inode *, struct file *);
static int bioshd_ioctl(struct inode *, struct file *, unsigned int, unsigned int);
static void bioshd_geninit(void);

static struct gendisk bioshd_gendisk = {
    MAJOR_NR,                   /* Major number */
    "hd",                       /* Major name */
    MINOR_SHIFT,                /* Bits to shift to get real from partition */
    1 << MINOR_SHIFT,           /* Number of partitions per real */
    NUM_DRIVES,                 /* maximum number of drives */
    bioshd_geninit,             /* init function */
    hd,                         /* hd struct */
    0,                          /* hd drives found */
    drive_info,
    NULL                        /* next */
};

static void set_cache_invalid(void)
{
    cache_drive = NULL;
}

#ifdef CONFIG_BLK_DEV_BFD
static int read_sector(int drive, int cylinder, int sector)
{
    int count = 2;              /* one retry on probe or boot sector read */

#ifdef CONFIG_ARCH_PC98
    drive = bios_drive_map[DRIVE_FD0+drive];
#endif

    set_cache_invalid();
    do {
        set_irq();
        bios_set_ddpt(36);      /* set to large value to avoid BIOS issues*/
        if (!bios_disk_rw(BIOSHD_READ, 1, drive, cylinder, 0, sector, DMASEG, 0))
            return 0;           /* everything is OK */
        bios_disk_reset(drive);
    } while (--count > 0);
    return 1;                   /* error */
}

static void probe_floppy(int target, struct hd_struct *hdp)
{
    /* Check for disk type */

    /* I guess it works now as it should. Tested under dosemu with 720Kb,
     * 1.2 MB and 1.44 MB floppy image and works fine - Blaz Antonic
     */
    if (target >= DRIVE_FD0) {          /* the floppy drives */
        register struct drive_infot *drivep = &drive_info[target];

        /* probing range can be easily extended by adding more values to these
         * two lists and adjusting for loop' parameters in line 433 and 446 (or
         * somewhere near)
         */
#ifdef CONFIG_ARCH_PC98
        static unsigned char sector_probe[3] = { 8, 9, 18 };
        static unsigned char track_probe[2] = { 77, 80 };
#else
        static unsigned char sector_probe[5] = { 8, 9, 15, 18, 36 };
        static unsigned char track_probe[2] = { 40, 80 };
#endif
        int count, found_PB = 0;

        target &= FD_DRIVES - 1;

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

                if (drivep->cylinders != 0 && drivep->sectors != 0 &&
                    drivep->heads != 0) {
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
                //(boot[510] == 0x55 && boot[511] == 0xAA) &&   /* bootable sig*/
                ((boot[3] == 'M' && boot[4] == 'S') ||          /* OEM 'MSDOS'*/
                 (boot[3] == 'I' && boot[4] == 'B'))     &&     /* or 'IBM'*/
                (boot[54] == 'F' && boot[55] == 'A')       ) {  /* v4.0 fil_sys 'FAT'*/

                /* has valid MSDOS 4.0+ FAT BPB, use it */
                drivep->sectors = boot[24];             /* bpb_sec_per_trk */
                drivep->heads = boot[26];               /* bpb_num_heads */
                unsigned char media = boot[21];         /* bpb_media_byte */
                drivep->cylinders =
                        (media == 0xFD)? 40:
#ifdef CONFIG_ARCH_PC98
                        (media == 0xFE)? 77:            /* FD1232 is 77 tracks */
#endif
                                         80;
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
        int pc98_720KB = 0;
        do {
            if (count)
                bios_switch_device98(target, 0x30, drivep);  /* 1.44 MB */
            /* skip probing first entry */
            if (count && read_sector(target, track_probe[count] - 1, 1)) {
                bios_switch_device98(target, 0x10, drivep);  /* 720 KB */
                if (read_sector(target, track_probe[count] - 1, 1))
                    bios_switch_device98(target, 0x90, drivep);  /* 1.232 MB */
                else
                    pc98_720KB = 1;
            }
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
            if (count == 2)
                bios_switch_device98(target, 0x30, drivep);  /* 1.44 MB */
            /* skip reading first entry */
            if ((count == 2) && read_sector(target, 0, sector_probe[count])) {
                if (pc98_720KB) {
                    bios_switch_device98(target, 0x10, drivep);  /* 720 KB */
                    /* Read BPB to find 8 sectors, 640KB format. Currently, it is not supported */
                    unsigned char __far *boot = _MK_FP(DMASEG, 0);
                    if (!read_sector(target, 0, 1) && (boot[24] == 8))
                        bios_switch_device98(target, 0x90, drivep);
                }
                else
                    bios_switch_device98(target, 0x90, drivep);  /* 1.232 MB */
            }
        } while (++count < sizeof(sector_probe)/sizeof(sector_probe[0]));
#else
        do {
            int res = read_sector(target, 0, sector_probe[count]);
#if DEBUG_PROBE
            printk("SEC %d %s, ", sector_probe[count], res? "fail": "ok");
#endif
            if (res) {
                if (count == 0) {       /* failed on first sector read, use BIOS parms */
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
        printk("fd%d: %s has %d cylinders, %d heads, and %d sectors\n", target,
            (found_PB == 2)? "DOS format," :
            (found_PB == 1)? "ELKS bootable,": "probed, probably",
            drivep->cylinders, drivep->heads, drivep->sectors);
        hdp->start_sect = 0;
        hdp->nr_sects = ((sector_t)(drivep->sectors * drivep->heads)) *
                        ((sector_t)drivep->cylinders);
    }
}
#endif /* CONFIG_BLK_DEV_BFD*/

static int bioshd_open(struct inode *inode, struct file *filp)
{
    int target = DEVICE_NR(inode->i_rdev);      /* >> MINOR_SHIFT */
    struct hd_struct *hdp = &hd[MINOR(inode->i_rdev)];

    if (!bioshd_initialized || target >= NUM_DRIVES || hdp->start_sect == -1U)
        return -ENXIO;

    if (++access_count[target] == 1) {
#ifdef CONFIG_BLK_DEV_BFD
        probe_floppy(target, hdp);      /* probe only on initial open */
#endif
    }
    inode->i_size = hdp->nr_sects * drive_info[target].sector_size;
    /* limit inode size to max filesize for CHS >= 4MB (2^22)*/
    if (hdp->nr_sects >= 0x00400000L)   /* 2^22*/
        inode->i_size = 0x7ffffffL;     /* 2^31 - 1*/
    return 0;
}

static void bioshd_release(struct inode *inode, struct file *filp)
{
    kdev_t dev = inode->i_rdev;
    int target = DEVICE_NR(dev);

    if (--access_count[target] == 0) {
        fsync_dev(dev);
        invalidate_inodes(dev);
        invalidate_buffers(dev);
    }
}

static struct file_operations bioshd_fops = {
    NULL,                       /* lseek - default */
    block_read,                 /* read - general block-dev read */
    block_write,                /* write - general block-dev write */
    NULL,                       /* readdir */
    NULL,                       /* select */
    bioshd_ioctl,               /* ioctl */
    bioshd_open,                /* open */
    bioshd_release              /* release */
};

void INITPROC bioshd_init(void)
{
    register struct gendisk *ptr;
    int count;

    /* FIXME perhaps remove for speed on floppy boot*/
    outb_p(0x0C, FDC_DOR);      /* FD motors off, enable IRQ and DMA*/

#if defined(CONFIG_BLK_DEV_BFD) || defined(CONFIG_BLK_DEV_BFD_HARD)
    fd_count = bios_getfdinfo(&drive_info[DRIVE_FD0]);
#endif
#ifdef CONFIG_BLK_DEV_BHD
    hd_count = bios_gethdinfo(&drive_info[DRIVE_HD0]);
    bioshd_gendisk.nr_hd = hd_count;
#endif

#ifdef PER_DRIVE_INFO
    {
        register struct drive_infot *drivep;
        static char UNITS[4] = "KMGT";

        drivep = drive_info;
        for (count = 0; count < NUM_DRIVES; count++, drivep++) {
            if (drivep->heads != 0) {
                char *unit = UNITS;
                __u32 size = ((__u32) drivep->sectors) * 5;     /* 0.1 kB units */
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
                printk("%cd%c: %4lu%c CHS %3u,%2d,%d\n",
                    (count < 4 ? 'h' : 'f'), (count & 3) + (count < 4 ? 'a' : '0'),
                    (size/10), *unit,
                    drivep->cylinders, drivep->heads, drivep->sectors);
            }
        }
    }
#else /* one line version */
#ifdef CONFIG_BLK_DEV_BFD
#ifdef CONFIG_BLK_DEV_BHD
    printk("bioshd: %d floppy drive%s and %d hard drive%s\n",
           fd_count, fd_count == 1 ? "" : "s",
           hd_count, hd_count == 1 ? "" : "s");
#else
    printk("bioshd: %d floppy drive%s\n",
           fd_count, fd_count == 1 ? "" : "s");
#endif
#else
#ifdef CONFIG_BLK_DEV_BHD
    printk("bioshd: %d hard drive%s\n",
           hd_count, hd_count == 1 ? "" : "s");
#endif
#endif
#endif /* PER_DRIVE_INFO */

    if (!(fd_count + hd_count)) return;

    bios_copy_ddpt();       /* make a RAM copy of the disk drive parameter table*/

    if (!register_blkdev(MAJOR_NR, DEVICE_NAME, &bioshd_fops)) {
        blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;

        if (gendisk_head == NULL) {
            bioshd_gendisk.next = gendisk_head;
            gendisk_head = &bioshd_gendisk;
        } else {
            for (ptr = gendisk_head; ptr->next != NULL; ptr = ptr->next)
                continue;
            ptr->next = &bioshd_gendisk;
            //bioshd_gendisk.next = NULL;
        }
        bioshd_initialized = 1;
    } else {
        printk("bioshd: init error\n");
    }
}

static void INITPROC bioshd_geninit2(void)
{
    struct drive_infot *drivep = drive_info;
    struct hd_struct *hdp = hd;
    int i;

    for (i = 0; i < NUM_DRIVES << MINOR_SHIFT; i++) {
        if ((i & ((1 << MINOR_SHIFT) - 1)) == 0) {
            hdp->nr_sects = (sector_t)drivep->sectors * drivep->heads * drivep->cylinders;
            hdp->start_sect = 0;
            drivep++;
        } else {
            hdp->nr_sects = 0;
            hdp->start_sect = -1;
        }
        hdp++;
    }

}

/* called by setup_dev() */
static void bioshd_geninit(void)
{
    bioshd_geninit2();
}

static int bioshd_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
    unsigned int arg)
{
    register struct hd_geometry *loc = (struct hd_geometry *) arg;
    register struct drive_infot *drivep;
    int dev, err;

    /* get sector size called with NULL inode and arg = superblock s_dev */
    if (cmd == IOCTL_BLK_GET_SECTOR_SIZE)
        return drive_info[DEVICE_NR(arg)].sector_size;

    if (!inode || !inode->i_rdev)
        return -EINVAL;

    dev = DEVICE_NR(inode->i_rdev);
    if (dev >= ((dev < DRIVE_FD0) ? hd_count : (DRIVE_FD0 + fd_count)))
        return -ENODEV;

    drivep = &drive_info[dev];
    err = -EINVAL;
    switch (cmd) {
    case HDIO_GETGEO:
        err = verify_area(VERIFY_WRITE, (void *)loc, sizeof(struct hd_geometry));
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
static void get_chst(struct drive_infot *drivep, sector_t *start_sec, unsigned int *c,
        unsigned int *h, unsigned int *s, unsigned int *t, int fulltrack)
{
    sector_t start = *start_sec;
    sector_t tmp;

    *s = (unsigned int) (start % drivep->sectors) + 1;
    tmp = start / drivep->sectors;
    *h = (unsigned int) (tmp % drivep->heads);
    *c = (unsigned int) (tmp / drivep->heads);
    *t = drivep->sectors - *s + 1;
#if FULL_TRACK
    if (fulltrack) {
        int save = *s;
        int max_sectors = DMASEGSZ / drivep->sector_size;
        if (*s - 1 < max_sectors) { /* adjust start sector backwards for full track read*/
            *s = 1;
            *t = max_sectors;
        } else {                    /* likely 2880k: limit to size of DMASEG buffer */
            *s = max_sectors + 1;
            *t = drivep->sectors - *s + 1;
        }
        *start_sec -= save - *s;
    }
#endif
    debug_bios("bioshd: lba %ld is CHS %d/%d/%d remaining sectors %d\n",
        start, *c, *h, *s, *t);
}

/* do disk I/O, return # sectors read/written */
static int do_readwrite(struct drive_infot *drivep, sector_t start, char *buf,
        ramdesc_t seg, int cmd, unsigned int count)
{
    int drive, error, errs;
    unsigned int cylinder, head, sector, this_pass;
    unsigned int segment, offset;
    unsigned int physaddr;
    size_t end;
    int usedmaseg;

    drive = bios_drive_map[drivep - drive_info];
    get_chst(drivep, &start, &cylinder, &head, &sector, &this_pass, 0);

    /* limit I/O to requested sector count*/
    if (this_pass > count) this_pass = count;
    if (cmd == READ) debug_bios("bioshd(%x): read lba %ld count %d\n",
                        drive, start, this_pass);

    errs = MAX_ERRS;        /* BIOS disk reads should be retried at least three times */
    do {
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
        usedmaseg = seg >> 16; /* will be nonzero only if XMS configured and XMS buffer */
        if (!usedmaseg) {
            /* check for 64k I/O overlap */
            physaddr = (seg << 4) + (unsigned int)buf;
            end = this_pass * drivep->sector_size - 1;
            usedmaseg = (physaddr + end < physaddr);
            debug_blk("bioshd: %p:%p = %p count %d wrap %d\n",
                (unsigned int)seg, buf, physaddr, this_pass, usedmaseg);
        }
        if (usedmaseg) {
            segment = DMASEG;           /* if xms buffer use DMASEG*/
            offset = 0;
            if (cmd == WRITE)           /* copy xms buffer down before write*/
                xms_fmemcpyw(0, DMASEG, buf, seg, this_pass*(drivep->sector_size >> 1));
            set_cache_invalid();
        } else {
            segment = (seg_t)seg;
            offset = (unsigned) buf;
        }
        debug_bios("bioshd(%x): cmd %d CHS %d/%d/%d count %d\n",
            drive, cmd, cylinder, head, sector, this_pass);

        bios_set_ddpt(drivep->sectors);
        error = bios_disk_rw(cmd == WRITE? BIOSHD_WRITE: BIOSHD_READ, this_pass,
                                drive, cylinder, head, sector, segment, offset);
        if (error) {
            printk("bioshd(%x): cmd %d retry #%d CHS %d/%d/%d count %d\n",
                drive, cmd, MAX_ERRS - errs + 1, cylinder, head, sector, this_pass);
            bios_disk_reset(drive);
        }
    } while (error && --errs);      /* On error, retry up to MAX_ERRS times */
    last_drive = drivep;

    if (error) return 0;            /* error message in blk.h */

    if (usedmaseg) {
        if (cmd == READ)            /* copy DMASEG up to xms*/
            xms_fmemcpyw(buf, seg, 0, DMASEG, this_pass*(drivep->sector_size >> 1));
        set_cache_invalid();
    }
    return this_pass;
}

#ifdef CONFIG_TRACK_CACHE               /* use track-sized sector cache*/
static sector_t cache_startsector;
static sector_t cache_endsector;

/* read from start sector to end of track into DMASEG track buffer, no retries*/
static void do_readtrack(struct drive_infot *drivep, sector_t start)
{
    unsigned int cylinder, head, sector, num_sectors;
    int drive = drivep - drive_info;
    int error, errs = 0;

    drive = bios_drive_map[drive];
    get_chst(drivep, &start, &cylinder, &head, &sector, &num_sectors, 1);

    if (num_sectors > (DMASEGSZ / drivep->sector_size))
        num_sectors = DMASEGSZ / drivep->sector_size;

    do {
        debug_bios("bioshd(%x): track read CHS %d/%d/%d count %d\n",
                drive, cylinder, head, sector, num_sectors);

        bios_set_ddpt(drivep->sectors);
        error = bios_disk_rw(BIOSHD_READ, num_sectors, drive,
                                 cylinder, head, sector, DMASEG, 0);
        if (error) {
            printk("bioshd(%x): track read retry #%d CHS %d/%d/%d count %d\n",
                drive, errs + 1, cylinder, head, sector, num_sectors);
            bios_disk_reset(drive);
        }
    } while (error && ++errs < 1); /* no track retries, for testing only*/
    last_drive = drivep;

    if (error) {
        set_cache_invalid();
        return;
    }

    cache_drive = drivep;
    cache_startsector = start;
    cache_endsector = start + num_sectors - 1;
    debug_bios("bioshd(%x): track read lba %ld to %ld count %d\n",
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
    debug_bios("bioshd(%x): cache hit lba %ld\n",
        bios_drive_map[drivep-drive_info], start);
    xms_fmemcpyw(buf, seg, (void *)offset, DMASEG, drivep->sector_size >> 1);
    return 1;
}

static int cache_tries;
static int cache_hits;

/* read from cache, return # sectors read*/
static int do_cache_read(struct drive_infot *drivep, sector_t start, char *buf,
        ramdesc_t seg, int cmd)
{
    if (cmd == READ) {
        cache_tries++;
        if (cache_valid(drivep, start, buf, seg)) { /* try cache first*/
            cache_hits++;
            return 1;
        }
        do_readtrack(drivep, start);                /* read whole track*/
        if (cache_valid(drivep, start, buf, seg))   /* try cache again*/
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
    for (;;) {
next_block:

        req = CURRENT;
        if (!req)
            break;
        CHECK_REQUEST(req);

        if (bioshd_initialized != 1) {
            end_request(0);
            continue;
        }
        minor = MINOR(req->rq_dev);
        drive = minor >> MINOR_SHIFT;
        drivep = &drive_info[drive];

        /* make sure it's a disk that we are dealing with. */
        if (drive > (DRIVE_FD0 + FD_DRIVES - 1) || drivep->heads == 0) {
            printk("bioshd: non-existent drive\n");
            end_request(0);
            continue;
        }

        /* get request start sector and sector count */
        count = req->rq_nr_sectors;
        start = req->rq_sector;

        if (hd[minor].start_sect == -1U || start + count > hd[minor].nr_sects) {
            printk("bioshd: sector %ld access beyond partition (%ld,%ld)\n",
                start, hd[minor].start_sect, hd[minor].nr_sects);
            end_request(0);
            continue;
        }
        start += hd[minor].start_sect;

        buf = req->rq_buffer;
        while (count > 0) {
            int num_sectors = 0;
#ifdef CONFIG_TRACK_CACHE
            if (drivep - drive_info >= DRIVE_FD0) {
                /* first try reading track cache*/
                num_sectors = do_cache_read(drivep, start, buf, req->rq_seg, req->rq_cmd);
            }
            if (!num_sectors)
#endif
                /* then fallback with retries if required*/
                num_sectors = do_readwrite(drivep, start, buf, req->rq_seg,
                    req->rq_cmd, count);

            if (num_sectors == 0) {
                end_request(0);
                goto next_block;
            }

            count -= num_sectors;
            start += num_sectors;
            buf += num_sectors * drivep->sector_size;
        }
        debug_bios("cache: hits %u total %u %lu%%\n", cache_hits, cache_tries,
            (long)cache_hits * 100L / cache_tries);

        /* satisfied that request */
        end_request(1);
    }
    spin_timer(0);
}
