/*
 *  Code extracted from
 *  linux/kernel/hd.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *
 *  Thanks to Branko Lankester, lankeste@fwi.uva.nl, who found a bug
 *  in the early extended-partition checks and added DM partitions
 *
 *  Support for DiskManager v6.0x added by Mark Lord (mlord@bnr.ca)
 *  with information provided by OnTrack.  This now works for linux fdisk
 *  and LILO, as well as loadlin and bootln.  Note that disks other than
 *  /dev/hda *must* have a "DOS" type 0x51 partition in the first slot (hda1).
 *
 *  More flexible handling of extended partitions - aeb, 950831
 */

#include <linuxmt/config.h>
#include <linuxmt/fs.h>
#include <linuxmt/genhd.h>
#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/string.h>

#include <arch/system.h>

#define NR_SECTS(p)	p->nr_sects
#define START_SECT(p)	p->start_sect

struct gendisk *gendisk_head = NULL;

static int current_minor = 0;
#ifdef BDEV_SIZE_CHK
extern int blk_size[];
#endif
extern void rd_load();

extern int chr_dev_init();
extern int blk_dev_init();

#ifdef CONFIG_BLK_DEV_BHD
#define CONFIG_PART
#endif
#ifdef CONFIG_BLK_DEV_HD
#define CONFIG_PART
#endif

#ifdef CONFIG_PART
static void print_minor_name (hd,minor)
register struct gendisk *hd;
int minor;
{
	unsigned int unit = minor >> hd->minor_shift;
	unsigned int part = minor & ((1 << hd->minor_shift) - 1);

	printk(" %s%c", hd->major_name, 'a' + unit);
	if (part)
		printk("%d", part);
	else
		printk(":");
}

static void add_partition (hd,minor,start,size)
register struct gendisk *hd;
int minor;
unsigned long start;
unsigned long size;
{
	hd->part[minor].start_sect = start;
	hd->part[minor].nr_sects   = size;
	print_minor_name(hd, minor);
}

static int is_extended_partition(p)
struct partition *p;
{
	return (p->sys_ind == DOS_EXTENDED_PARTITION ||
		p->sys_ind == LINUX_EXTENDED_PARTITION);
}

/*
 * Create devices for each logical partition in an extended partition.
 * The logical partitions form a linked list, with each entry being
 * a partition table with two entries.  The first entry
 * is the real data partition (with a start relative to the partition
 * table start).  The second is a pointer to the next logical partition
 * (with a start relative to the entire extended partition).
 * We do not create a Linux partition for the partition tables, but
 * only for the actual data partitions.
 */

static void extended_partition(hd,dev)
register struct gendisk *hd;
kdev_t dev;
{
	struct buffer_head *bh;
	register struct partition *p;
	unsigned long first_sector, first_size, this_sector, this_size;
	int mask = (1 << hd->minor_shift) - 1;
	int i;

	first_sector = hd->part[MINOR(dev)].start_sect;
	first_size = hd->part[MINOR(dev)].nr_sects;
	this_sector = first_sector;

	while (1) {
		if ((current_minor & mask) == 0)
			return;
		if (!(bh = bread(dev,0L)))
			return;
	  /*
	   * This block is from a device that we're about to stomp on.
	   * So make sure nobody thinks this block is usable.
	   */
#ifdef BLOAT_FS
		bh->b_state = 0;
#endif

		map_buffer(bh);
		if (*(unsigned short *) (bh->b_data+510) != 0xAA55)
			goto done;

		p = (struct partition *) (0x1BE + bh->b_data);

		this_size = hd->part[MINOR(dev)].nr_sects;

		/*
		 * Usually, the first entry is the real data partition,
		 * the 2nd entry is the next extended partition, or empty,
		 * and the 3rd and 4th entries are unused.
		 * However, DRDOS sometimes has the extended partition as
		 * the first entry (when the data partition is empty),
		 * and OS/2 seems to use all four entries.
		 */

		/* 
		 * First process the data partition(s)
		 */
		for (i=0; i<4; i++, p++) {
		    if (!NR_SECTS(p) || is_extended_partition(p))
		      continue;

		    /* Check the 3rd and 4th entries -
		       these sometimes contain random garbage */
		    if (i >= 2
			&& START_SECT(p) + NR_SECTS(p) > this_size
			&& (this_sector + START_SECT(p) < first_sector ||
			    this_sector + START_SECT(p) + NR_SECTS(p) >
			     first_sector + first_size))
		      continue;

		    add_partition(hd, current_minor, (unsigned long)this_sector+START_SECT(p), (unsigned long)NR_SECTS(p));
		    current_minor++;
		    if ((current_minor & mask) == 0)
		      goto done;
		}
		/*
		 * Next, process the (first) extended partition, if present.
		 * (So far, there seems to be no reason to make
		 *  extended_partition()  recursive and allow a tree
		 *  of extended partitions.)
		 * It should be a link to the next logical partition.
		 * Create a minor for this just long enough to get the next
		 * partition table.  The minor will be reused for the next
		 * data partition.
		 */
		p -= 4;
		for (i=0; i<4; i++, p++)
		  if(NR_SECTS(p) && is_extended_partition(p))
		    break;
		if (i == 4)
		  goto done;	 /* nothing left to do */

		hd->part[current_minor].nr_sects = NR_SECTS(p);
		hd->part[current_minor].start_sect = first_sector + START_SECT(p);
		this_sector = first_sector + START_SECT(p);
		dev = MKDEV(hd->major, current_minor);
		unmap_brelse(bh);
	}
	return;
done:
	unmap_brelse(bh);
/*	unmap_buffer(bh);
	brelse(bh); */
}

static int msdos_partition(hd,dev,first_sector)
register struct gendisk *hd;
kdev_t dev;
unsigned long first_sector;
{
	int i, minor = current_minor;
	struct buffer_head *bh;
	register struct partition *p;
	unsigned char *data;
	int mask = (1 << hd->minor_shift) - 1;
	if (!(bh = bread(dev,0L))) {
		printk(" unable to read partition table\n");
		return -1;
	}
	map_buffer(bh);
	data = bh->b_data;
	/* In some cases we modify the geometry    */
	/*  of the drive (below), so ensure that   */
	/*  nobody else tries to re-use this data. */
#ifdef BLOAT_FS
	bh->b_state = 0;
#endif
	if (*(unsigned short *)  (0x1fe + data) != 0xAA55) {
		printk(" bad magic number\n");
		unmap_buffer(bh);
		brelse(bh);
		return 0;
	}
	p = (struct partition *) (0x1be + data);


	current_minor += 4;  /* first "extra" minor (for extended partitions) */
	for (i=1 ; i<=4 ; minor++,i++,p++) {
		if (!NR_SECTS(p))
		{
			continue;
		}
		add_partition(hd, minor, first_sector+START_SECT(p), NR_SECTS(p));
		if (is_extended_partition(p)) {
			printk(" <");
			/*
			 * If we are rereading the partition table, we need
			 * to set the size of the partition so that we will
			 * be able to bread the block containing the extended
			 * partition info.
			 */
			hd->sizes[minor] = hd->part[minor].nr_sects 
			  	>> (BLOCK_SIZE_BITS - 9);
			extended_partition(hd, MKDEV(hd->major, minor));
			printk(" >");
			/* prevent someone doing mkfs or mkswap on an
			   extended partition, but leave room for LILO */
			if (hd->part[minor].nr_sects > 2)
				hd->part[minor].nr_sects = 2;
		}
	}
	printk("\n");
	unmap_brelse(bh);
/*	unmap_buffer(bh);
	brelse(bh); */
	return 1;
}

void check_partition(hd,dev)
register struct gendisk *hd;
kdev_t dev;
{
	static int first_time = 1;
	unsigned long first_sector;

	if (first_time)
		printk("Partition check:\n");
	first_time = 0;
	first_sector = hd->part[MINOR(dev)].start_sect;

	/*
	 * This is a kludge to allow the partition check to be
	 * skipped for specific drives (e.g. IDE cd-rom drives)
	 */
#if 0
	if ((long)first_sector == -1) {
		hd->part[MINOR(dev)].start_sect = 0;
		return;
	}
#endif

	printk(" ");
	print_minor_name(hd, MINOR(dev));
#ifdef CONFIG_MSDOS_PARTITION
	if (msdos_partition(hd, dev, first_sector))
		return;
#endif
	printk(" unknown partition table\n");
}
#endif /* CONFIG_PART */

/* This function is used to re-read partition tables for removable disks.
   Much of the cleanup from the old partition tables should have already been
   done */

/* This function will re-read the partition tables for a given device,
and set things back up again.  There are some important caveats,
however.  You must ensure that no one is using the device, and no one
can start using the device while this function is being executed. */
#if 0 /* Currently unused */
void resetup_one_dev(dev,drive)
struct gendisk *dev;
int drive;
{
	int i;
	int first_minor	= drive << dev->minor_shift;
	int end_minor	= first_minor + dev->max_p;

	blk_size[dev->major] = NULL;
	current_minor = 1 + first_minor;
	check_partition(dev, MKDEV(dev->major, first_minor));
}
#endif 
static void setup_dev(dev)
register struct gendisk *dev;
{
	int i, drive;
	int end_minor	= dev->max_nr * dev->max_p;

#ifdef BDEV_SIZE_CHK
	blk_size[dev->major] = NULL;
#endif
	for (i = 0 ; i < end_minor; i++) {
		dev->part[i].start_sect = 0;
		dev->part[i].nr_sects = 0;
	}
	dev->init(dev);
#ifdef CONFIG_PART
	for (drive = 0 ; drive < dev->nr_real ; drive++) {
		int first_minor	= drive << dev->minor_shift;
		current_minor = 1 + first_minor;
		check_partition(dev, MKDEV(dev->major, first_minor));
	}
#endif
}

void device_setup()
{
	register struct gendisk *p;
	int nr=0;

	chr_dev_init();
	blk_dev_init();
	sti();

	for (p = gendisk_head ; p ; p=p->next) {
		setup_dev(p);
		nr += p->nr_real;
	}
#ifdef CONFIG_BLK_DEV_RAM
	rd_load();
#endif
}
