/*
 * NOTE: experimental direct hd driver - NOT WORKING
 *
 * directhd driver for ELKS kernel
 * Copyright (C) 1998 Blaz Antonic
 * 14.04.1998 Bugfixes by Alastair Bridgewater nyef@sudval.org
 */

#include <linuxmt/config.h>
#include <linuxmt/major.h>
#include <linuxmt/genhd.h>
#include <linuxmt/fs.h>
#include <linuxmt/string.h>
#include <linuxmt/mm.h>
#include <linuxmt/directhd.h>
#include <linuxmt/debug.h>

#include <arch/hdreg.h>
#include <arch/io.h>
#include <arch/segment.h>

/* maybe we should have word-wide input here instead of byte-wide ? */
#define STATUS(port) inb_p(port + DIRECTHD_STATUS)
#define ERROR(port) inb_p(port + DIRECTHD_ERROR)

#define WAITING(port) (STATUS(port) & 0x80) == 0x80

/* use asm insw/outsw instead of C version */
/* asm versions should work on 8088/86, but only with CONFIG_PC_XT */
/* #define USE_ASM */

/* uncomment this to include debugging code .. this increases size of driver */
#define USE_DEBUG_CODE

#define MAJOR_NR ATHD_MAJOR
#define ATDISK
#include "blk.h"

static int directhd_ioctl();
static int directhd_open();
static void directhd_release();

static struct file_operations directhd_fops = {
    NULL,			/* lseek */
    block_read,			/* read */
    block_write,		/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    directhd_ioctl,		/* ioctl */
    directhd_open,		/* open */
    directhd_release		/* release */
};

/* what is this good for ? */
static int access_count[4] = { 0, };

static int io_ports[2] = { HD1_PORT, HD2_PORT };

static int directhd_initialized = 0;

static struct drive_infot {
    int cylinders;
    int sectors;
    int heads;
} drive_info[4] = { 0, };	/* preset to 0 */

static struct hd_struct hd[4 << 6];
static void directhd_geninit();

static struct gendisk directhd_gendisk = {
    MAJOR_NR,			/* major number */
    "dhd",			/* device name */
    6,
    1 << 6,
    4,
    directhd_geninit,		/* init */
    hd,				/* hd struct */
    0,
    drive_info,
    NULL
};

void directhd_geninit(void)
{
    int i;

    for (i = 0; i < 4 << 6; i++) {
	if ((i & ((1 << 6) - 1)) == 0) {
	    hd[i].start_sect = 0;
	    hd[i].nr_sects = (sector_t)drive_info[i >> 6].sectors *
		drive_info[i >> 6].heads * drive_info[i >> 6].cylinders;
	} else {
	    hd[i].start_sect = -1;
	    hd[i].nr_sects = 0;
	}
    }
    return;
}

#ifndef USE_ASM

void insw(unsigned int port,unsigned int *buffer,int count)
{
    int i;

    for (i = 0; i < count / 2; i++)
	buffer[i] = inw(port);
    return;
}

#else

/* this _should_ work on an 8088/86 now, but I haven't tested it yet */

void insw(unsigned int port,unsigned int *buffer,int count)
{
#asm
	push	bp
	mov	bp, sp
	push	cx
	push	di
	push	dx
	mov	dx, [bp + 04]
	mov	di, [bp + 06]
	mov	cx, [bp + 08]
	shr	cx, 1
	cld
#ifndef CONFIG_PC_XT
	repz
	insw
#else
			/* this should work on an 8088 */
dhd_insw_loop:
	in	ax, dx
	stosw
	loop	dhd_insw_loop
#endif
	pop	dx
	pop	di
	pop	cx
	pop	bp
#endasm
}

#endif

#ifndef USE_ASM

void outsw(unsigned int port,unsigned int *buffer,int count)
{
    int i;

    for (i = 0; i < count / 2; i++)
	outw(port, buffer[i]);
    return;
}

#else

/* the assembler version. Again, this should work,
 * but I haven't had a chance to test it yet.
 */

void outsw(unsigned int port,unsigned int *buffer,int count)
{
#asm
	push	bp
	mov	bp, sp
	push	cx
	push	si
	push	dx
	mov	dx, [bp + 04]
	mov	si, [bp + 06]
	mov	cx, [bp + 08]
	shr	cx, 1
	cld
#ifndef CONFIG_PC_XT
	repz
	outsw
#else
dhd_outsw_loop:		/* this should work on an 8088 */
	lodsw
	out	dx, ax
	loop	dhd_outsw_loop
#endif
	pop	dx
	pop	si
	pop	cx
	pop	bp
#endasm
}

#endif

#if 0				/* not used */

void swap_order(unsigned char *buffer,int count)
{
    int i;
    char tmp;

    for (i = 0; i < count; i++)
	if ((i % 2) == 0) {
	    tmp = *(buffer + i + 1);
	    *(buffer + i + 1) = *(buffer + i);
	    *(buffer + i) = tmp;
	}
    return;
}

#endif

void out_hd(unsigned int drive,unsigned int nsect,unsigned int sect,
	    unsigned int head,unsigned int cyl,unsigned int cmd)
{
    unsigned int port;

    port = io_ports[drive / 2];
    /* setting WPCOM to 0 is not good. this change uses the last value input to
     * the drive. (my BIOS sets this correctly, so it works for now but we should
     * fix this to work properly)  -- Alastair Bridgewater */
    /* this doesn't matter on newer (IDE) drives, but is important for MFM/RLLs
     * I'll add support for those later and we'll need it then - Blaz Antonic */
    /* meanwhile, I found some documentation that says that for IDE drives
     * the correct WPCOM value is 0xff. so I changed it.
     * -- Alastair Bridewater */
#if 0
    outb_p(0, ++port);	/* wr_precompensation .. */
    port++;
#endif
    outb(0xff, ++port);		/* the supposedly correct value for WPCOM on IDE */
    outb_p(nsect, ++port);
    outb_p(sect, ++port);
    outb_p(cyl, ++port);
    outb_p(cyl >> 8, ++port);
    outb_p(0xA0 | ((drive % 2) << 4) | head, ++port);
    outb_p(cmd, ++port);

    return;
}

int directhd_init(void)
{
    unsigned int buffer[256];
    struct gendisk *ptr;
    int i, j, hdcount = 0, drive;
    unsigned int port;

    /* .. once for each drive */
    /* note, however, that this breaks if you don't have two IDE interfaces
     * in your computer. If you only have one, change the 4 to a 2.
     * (this explains why your computer was locking up after mentioning the
     * serial port, doesn't it? :-) -- Alastair Bridgewater */
    /* this should work now, IMO - Blaz Antonic */
    for (drive = 0; drive < 2; drive++) {
	/* send drive_ID command to drive */
	out_hd(drive, 0, 0, 0, 0, DIRECTHD_DRIVE_ID);

	port = io_ports[drive / 2];

	/* wait */
	while (WAITING(port));

	if ((STATUS(port) & 1) == 1) {
	    /* error - drive not found or non-ide */
#ifdef USE_DEBUG_CODE
	    printk("athd: drive %d (%d on port 0x%x) not found\n", drive,
		   drive / 2, port);
#endif
	    continue;		/* this jumps to the start of for loop and
				 * proceeds with checking for other drives
				 */
	    /* this could use some optimisation as it is unlikely for
	     * computer to have slave drive on channel where master is
	     * missing
	     *
	     * Actually, that does occur, so any such optimisation is most
	     * probably just plain broken - Riley Williams, April 2002.
	     */
	}

	/* get drive info */

	insw(port, buffer, 512);
#if 0
	swap_order(buffer, 512);
#endif

#if 0			/* this is a bug - read CD info few lines lower */
	if (!*buffer) {
	    /* unexpected 0; drive doesn't exist ? */
	    /* something went wrong */
	    debug("athd: drive not found\n");
	    break;

	}
#endif
	/* Gather useful info
	 * FIXME: LBA support will be added here, later .. maybe ..
	 * MFM/RLL support will be added after that .. maybe
	 *
	 * Safety check - check for heads returned and assume CD
	 * if we get typical CD response .. this is a good place for
	 * ATAPI ID, but it'd only enlarge the kernel and no HD can
	 * have 60416 heads :-))) .. 4096 heads should cover it,
	 * IDE will be dead by that time
	 *
	 * Maybe we can assume some specific number here ? i always
	 * get 60416 with ATAPI (Mitsumi) CD - Blaz Antonic
	 *
	 * Safety check - head, cyl and sector values must be other than
	 * 0 and buffer has to contain valid data (first entry in buffer
	 * must be other than 0)
	 *
	 * FIXME: This will cause problems on many new drives .. some
	 * of them even have more than 16.384 cylinders
	 *
	 * This is some sort of bugfix, we will use same method as real Linux -
	 * work with disk geometry set in current translation mode rather than
	 * using physical drive info. Physical info might correspond to logical
	 * info for some drives (those which don't support/use LBA mode
	 */

	if ((buffer[0x6c / 2] < 34096) && (*buffer != 0)
	    && (buffer[0x6c / 2] != 0) && (buffer[0x6e / 2] != 0)
	    && (buffer[0x70 / 2] != 0)) {
	    /* Physical value offsets: cyl 0x2, heads 0x6, sectors 0C */
	    drive_info[drive].cylinders = buffer[0x6c / 2];
	    drive_info[drive].heads = buffer[0x6e / 2];
	    drive_info[drive].sectors = buffer[0x70 / 2];

	    hdcount++;
	}
    }

    if (!hdcount) {
	printk("athd: no drives found\n");
	return 0;
    }

    directhd_gendisk.nr_hd = hdcount;

    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &directhd_fops)) {
	printk("athd: unable to register\n");
	return -1;
    }

    blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
    if (gendisk_head == NULL) {
	directhd_gendisk.next = gendisk_head;
	gendisk_head = &directhd_gendisk;
    } else {
	for (ptr = gendisk_head; ptr->next != NULL; ptr = ptr->next);
	ptr->next = &directhd_gendisk;
	directhd_gendisk.next = NULL;
    }

    printk("athd: found %d hard drive%c\n", hdcount, hdcount == 1 ? ' ' : 's');

    /* print drive info */
    for (i = 0; i < 4; i++)
	/* sanity check */
	if (drive_info[i].heads != 0) {
	    printk("athd: /dev/dhd%c: %d heads, %d cylinders, %d sectors\n",
		   (i + 'a'),
		   drive_info[i].heads,
		   drive_info[i].cylinders, drive_info[i].sectors);
	}

    directhd_initialized = 1;

    return 0;
}

/* why is arg unsigned int here if it's used as hd_geometry later ?
 * one of joys of K&R ? Someone please answer ...
 */
static int directhd_ioctl(struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned int arg)
{
    struct hd_geometry *loc = (struct hd_geometry *) arg;
    int dev, err;

    if ((!inode) || !(inode->i_rdev))
	return -EINVAL;

    dev = DEVICE_NR(inode->i_rdev);
    if (dev >= MAX_DRIVES)
	return -ENODEV;

    switch (cmd) {
    case HDIO_GETGEO:
	/* safety check .. i presume :) */
	err = verify_area(VERIFY_WRITE, arg, sizeof(*loc));
	if (err)
	    return err;

	put_user(drive_info[dev].heads, &loc->heads);
	put_user(drive_info[dev].sectors, &loc->sectors);
	put_user(drive_info[dev].cylinders, &loc->cylinders);
	put_user(hd[MINOR(inode->i_rdev)].start_sect, &loc->start);

	return 0;
	break;
    }

    return -EINVAL;
}

static int directhd_open(struct inode *inode, struct file *filp)
{
    unsigned int minor;
    int target = DEVICE_NR(inode->i_rdev);

    if (target >= 4 || !directhd_initialized)
	return -ENXIO;
    minor = MINOR(inode->i_rdev);
    if (((int) hd[minor].start_sect) == -1)
	return -ENXIO;

    access_count[target]++;

    /* something should be here, but can't remember what :)
     * it really isn't important until probe code works
     *
     * probe code works now but i still can't remember what is missing.
     * any clues ?
     */

    inode->i_size = (hd[minor].nr_sects) << 9;
    return 0;
}

static void directhd_release(struct inode *inode, struct file *filp)
{
    int target = DEVICE_NR(inode->i_rdev);

    access_count[target]--;

    /* nothing really to do */

    return;
}

void do_directhd_request(void)
{
    sector_t count;		/* # of sectors to read/write */
    int this_pass;		/* # of sectors read/written */
    sector_t start;		/* first sector */
    char *buff;			/* max_sect * sector_size, 63 * 512 bytes; it seems that all requests are only 1024 bytes */
    short sector;		/* 1 .. 63 ? */
    short cylinder;		/* 0 .. 1024 and maybe more */
    short head;			/* 0 .. 16 */
    unsigned int tmp;
    int minor;
    int drive;			/* 0 .. 3 */
    unsigned char i;		/* 0 .. (sectors per track - 1) .. 62 or less */
    unsigned char j;		/* 0 .. 255 */
    int port;

    while (1) {			/* process HD requests */
	struct request *req = CURRENT;
	if (!req)
	    return;
	CHECK_REQUEST(req);

	if (directhd_initialized != 1) {
	    end_request(0);
	    continue;
	}

	minor = MINOR(req->rq_dev);
	drive = minor >> 6;

	/* check if drive exists */
	if (drive > 3 || drive < 0 || drive_info[drive].heads == 0) {
	    printk("Non-existent drive\n");
	    end_request(0);
	    continue;
	}

	count = req->rq_nr_sectors;
	start = req->rq_sector;
	buff = req->rq_buffer;

	/* safety check should be here */

	if (hd[minor].start_sect == -1 || hd[minor].nr_sects < start) {
	    printk("Bad partition start\n");
	    end_request(0);
	    continue;
	}

	start += hd[minor].start_sect;

	while (count > 0) {
	    sector = (start % drive_info[drive].sectors) + 1;
	    tmp = start / drive_info[drive].sectors;
	    head = tmp % drive_info[drive].heads;
	    cylinder = tmp / drive_info[drive].heads;

#if 0
	    this_pass = count;	/* this is redundant */
#endif
	    if (count <= (drive_info[drive].sectors - sector + 1))
		this_pass = count;
	    else
		this_pass = drive_info[drive].sectors - sector + 1;

#ifdef USE_DEBUG_CODE
	    printk
		("athd: drive: %d cylinder: %d head: %d sector: %d start: %d\n",
		 drive, cylinder, head, sector, start);
#endif

	    port = io_ports[drive / 2];

	    if (req->rq_cmd == READ) {
#ifdef USE_DEBUG_CODE
		printk("athd: drive: %d this_pass: %d sector: %d head: %d\n", drive, this_pass, sector, head);
		printk("athd: cyl: %d start: %ld tmp: %d count: %ld di[d].s: %d di[0].s: %d\n", cylinder, start, tmp, count, drive_info[drive].sectors, drive_info[0].sectors);
#endif
		/* read to buffer */
		while (WAITING(port));
		/* send drive parameters */
		out_hd(drive, this_pass, sector, head, cylinder,
		       DIRECTHD_READ);

		/* wait */
		while (WAITING(port)) {
#ifdef USE_DEBUG_CODE
		    //printk("athd: statusa: 0x%x\n", STATUS(port));
#endif
		}
		if ((STATUS(port) & 1) == 1) {

		    /* something went wrong */
		    printk("athd: status: 0x%x error: 0x%x\n", STATUS(port),
			   ERROR(port));

		    break;
#if 0
 		    end_request(0);
#endif
		}

		tmp = 0x00;
		while ((tmp & 0x08) != 0x08) {
		    if ((tmp & 1) == 1) {

			printk("athd: status: 0x%x error: 0x%x\n",
			       STATUS(port), ERROR(port));

			break;
			end_request(0);
		    } else {
			tmp = STATUS(port);
#ifdef USE_DEBUG_CODE
			//printk("athd: statusb: 0x%x\n", tmp);
#endif
		    }
		}


		/* read this_pass * 512 bytes, which is 63 * 512 b max. */
		insw(port, buff, this_pass * 512);
	    }
	    if (req->rq_cmd == WRITE) {
		/* write from buffer */
		while (WAITING(port));
		/* send drive parameters */
		out_hd(drive, this_pass, sector, head, cylinder,
		       DIRECTHD_WRITE);

		/* wait */
		while (WAITING(port)) {
#ifdef USE_DEBUG_CODE
		    //printk("athd: statusa: 0x%x\n", STATUS(port));
#endif
		}
		if ((STATUS(port) & 1) == 1) {

		    /* something went wrong */
		    printk("athd: status: 0x%x error: 0x%x\n", STATUS(port),
			   ERROR(port));

		    break;
#if 0
		    end_request(0);
#endif
		}

		tmp = 0x00;
		while ((tmp & 0x08) != 0x08) {
		    if ((tmp & 1) == 1) {

			printk("athd: status: 0x%x error: 0x%x\n",
			       STATUS(port), ERROR(port));

			break;
			end_request(0);
		    } else {
			tmp = STATUS(port);
#ifdef USE_DEBUG_CODE
			//printk("athd: statusb: 0x%x\n", tmp);
#endif
		    }
		}

		outsw(port, buff, this_pass * 512);
	    }

	    count -= this_pass;
	    start += this_pass;
	    buff += this_pass * 512;
	}
	end_request(1);
    }
    return;
}
