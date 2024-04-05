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
#include <linuxmt/init.h>
#include <linuxmt/fs.h>
#include <linuxmt/genhd.h>
#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/string.h>
#include <linuxmt/memory.h>

#include <arch/system.h>

#include "blk.h"

#define NR_SECTS(p)     p->nr_sects
#define START_SECT(p)   p->start_sect
#ifdef CONFIG_ARCH_PC98
# define NR_SECTS_PC98(p98)     ((sector_t) END_SECT_PC98(p98) - START_SECT_PC98(p98) + 1)
# define START_SECT_PC98(p98)   ((sector_t) p98->cyl * last_drive->heads * last_drive->sectors + p98->head * last_drive->sectors + p98->sector)
# define END_SECT_PC98(p98)     ((sector_t) p98->end_cyl * last_drive->heads * last_drive->sectors + p98->end_head * last_drive->sectors + p98->end_sector)
#endif

struct gendisk *gendisk_head;
int boot_partition = 0;         /* MBR boot partition, if any*/

#ifdef CONFIG_BLK_DEV_BHD

static unsigned int current_minor;

static void INITPROC print_minor_name(register struct gendisk *hd, unsigned int minor)
{
    unsigned int part;
    struct hd_struct *hdp = &hd->part[minor];

    printk("%s%c", hd->major_name, 'a' + (unsigned char)(minor >> hd->minor_shift));
    if ((part = (unsigned) (minor & ((1 << hd->minor_shift) - 1))))
        printk("%d", part);
    printk(":(%lu,%lu) ", hdp->start_sect, hdp->nr_sects);
}

static void INITPROC add_partition(struct gendisk *hd, unsigned int minor,
                          sector_t start, sector_t size)
{
    struct hd_struct *hdp = &hd->part[minor];

    hdp->start_sect = start;
    hdp->nr_sects = size;
    print_minor_name(hd, minor);

    /*
     * Additional partition check since no MBR signature:
     * Some BIOS subtract a cylinder, making direct comparison incorrect.
     * A CHS cylinder can have 63 max sectors * 255 heads, so adjust for that.
     */
#if UNUSED /* partition skipping disabled as virtual cylinder values sometimes needed for CF cards*/
    struct hd_struct *hd0 = &hd->part[0];
    sector_t adj_nr_sects = hd0->nr_sects + 63 * 255;
    if (start > adj_nr_sects || start+size > adj_nr_sects) {
        printk("skipped ");
        hdp->start_sect = -1;
        hdp->nr_sects = 0;
        return;
    }
#endif

    /*
     * Save boot partition # based on start offset.  This is needed if
     * ROOT_DEV is still a BIOS drive number at this point (see init.c), and
     * if we have not yet figured out which partition is the boot partition.
     *
     * If the root device is already fully known, i.e. ROOT_DEV is already
     * a device number, then we do not really need boot_partition.
     */
    if (ROOT_DEV == bios_drive_map[minor >> hd->minor_shift]) {
        sector_t boot_start = SETUP_PART_OFFSETLO | (sector_t) SETUP_PART_OFFSETHI << 16;
        if (start == boot_start)
            boot_partition = minor & 0x7;
    }
}

static int INITPROC is_extended_partition(register struct partition *p)
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

static void INITPROC extended_partition(register struct gendisk *hd, kdev_t dev)
{
    struct buffer_head *bh;
    register struct partition *p;
    struct hd_struct *hdp = &hd->part[MINOR(dev)];
    sector_t first_sector, first_size, this_sector, this_size;
    int i, mask = (1 << hd->minor_shift) - 1;

    first_sector = hdp->start_sect;
    first_size = hdp->nr_sects;
    this_sector = first_sector;

    while (1) {
        if (((current_minor & mask) == 0) || !(bh = bread(dev, (block_t) 0)))
            return;
        /*
         * This block is from a device that we're about to stomp on.
         * So make sure nobody thinks this block is usable.
         */

        map_buffer(bh);
        if (*(unsigned short *) (bh->b_data + 510) != 0xAA55)
            goto done;

        p = (struct partition *) (0x1BE + bh->b_data);

        this_size = hdp->nr_sects;

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
        for (i = 0; i < 4; i++, p++) {
            if (!NR_SECTS(p) || is_extended_partition(p))
                continue;

            /* Check the 3rd and 4th entries -
             * these sometimes contain random garbage */
            if (i >= 2
                && START_SECT(p) + NR_SECTS(p) > this_size
                && (this_sector + START_SECT(p) < first_sector ||
                    this_sector + START_SECT(p) + NR_SECTS(p) >
                    first_sector + first_size)) continue;

            add_partition(hd, current_minor,
                          (sector_t) this_sector + START_SECT(p),
                          (sector_t) NR_SECTS(p));
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
         * Create a minor for this just enough to get the next
         * partition table.  The minor will be reused for the next
         * data partition.
         */
        p -= 4;
        for (i = 0; i < 4; i++, p++)
            if (NR_SECTS(p) && is_extended_partition(p))
                break;
        if (i == 4)
            goto done;          /* nothing left to do */

        hd->part[current_minor].nr_sects = NR_SECTS(p);
        hd->part[current_minor].start_sect = first_sector + START_SECT(p);
        this_sector = first_sector + START_SECT(p);
        dev = MKDEV(hd->major, current_minor);
        unmap_brelse(bh);
    }
    return;

  done:
    unmap_brelse(bh);
}

#ifdef CONFIG_MSDOS_PARTITION
static int INITPROC msdos_partition(struct gendisk *hd,
                           kdev_t dev, sector_t first_sector)
{
    struct buffer_head *bh;
    register struct partition *p;
#ifdef CONFIG_ARCH_PC98
    struct partition_pc98 *p98;
#endif
    register struct hd_struct *hdp;
    unsigned int i, minor = current_minor;

    if (!(bh = bread(dev, (block_t) 0)))
        return 0;
    map_buffer(bh);

    /* In some cases we modify the geometry of the drive (below), so ensure
     * that nobody else tries to re-use this data.
     */
    if (*(unsigned short *) (bh->b_data + 0x1fe) != 0xAA55) {
out:
        printk("no mbr,");
        unmap_brelse(bh);
        return 0;
    }

    /* verify MBR by checking for four valid partition entries*/
    i = 0;
    p = (struct partition *) (bh->b_data + 0x1be);
    for ( ; p < (struct partition *) (bh->b_data + 0x1fe); p++) {
        /* if invalid partition entry, assume no MBR*/
        if (p->boot_ind != 0x00 && p->boot_ind != 0x80)
            goto out;
        i += p->boot_ind;
    }
    /* if all partitions set inactive, check possible ELKS EPB signature 'eL'*/
    if (i == 0 && (*(unsigned short *) (bh->b_data + 0x1fc)) == 0x4c65)
        goto out;

    /* first "extra" minor (for extended partitions) */
    p = (struct partition *) (bh->b_data + 0x1be);
    current_minor += 4;
    for (i = 1; i <= 4; minor++, i++, p++) {
        hdp = &hd->part[minor];
        if (!NR_SECTS(p))
            continue;
        add_partition(hd, minor, first_sector + START_SECT(p), NR_SECTS(p));
        if (is_extended_partition(p)) {
            printk(" <");
            /*
             * If we are rereading the partition table, we need
             * to set the size of the partition so that we will
             * be able to bread the block containing the extended
             * partition info.
             */
#if UNUSED
            hd->sizes[minor] = hdp->nr_sects >> (BLOCK_SIZE_BITS - 9);
#endif
            extended_partition(hd, MKDEV(hd->major, minor));
            printk(" >");
            /* prevent someone doing mkfs on an
             * extended partition, but leave room for LILO */
            if (hdp->nr_sects > 2)
                hdp->nr_sects = 2;
        }
    }

#ifdef CONFIG_ARCH_PC98
    if (*(unsigned short *) (bh->b_data + 0x4) == 0x5049 &&
        *(unsigned short *) (bh->b_data + 0x6) == 0x314C) {
        printk(" pc-98 IPL1");
        current_minor -= 4;
        minor = current_minor;
        p98 = (struct partition_pc98 *) (bh->b_data + 0x200);
        current_minor += 4;
        for (i = 1; i <= 4; minor++, i++, p98++) {
            if (!START_SECT_PC98(p98))
                continue;

            add_partition(hd, minor, first_sector + START_SECT_PC98(p98),
                NR_SECTS_PC98(p98));
        }
    }
#endif
    printk("\n");
    unmap_brelse(bh);
    return 1;
}
#endif

static void INITPROC check_partition(register struct gendisk *hd, kdev_t dev)
{
    sector_t first_sector;

    first_sector = hd->part[MINOR(dev)].start_sect;

#if UNUSED
    /*
     * This is a kludge to allow the partition check to be
     * skipped for specific drives (e.g. IDE cd-rom drives)
     */
    if ((sector_t) first_sector == (sector_t) -1) {
        hd->part[MINOR(dev)].start_sect = (sector_t) 0;
        return;
    }
#endif

    print_minor_name(hd, MINOR(dev));

#ifdef CONFIG_MSDOS_PARTITION
    if (msdos_partition(hd, dev, first_sector))
        return;
#endif

    printk(" no partitions\n");
}
#endif

void INITPROC setup_dev(register struct gendisk *dev)
{
    //memset((void *)dev->part, 0, sizeof(struct hd_struct)*dev->max_nr*dev->max_p);
    dev->init();

#ifdef CONFIG_BLK_DEV_BHD
    for (int i = 0; i < dev->nr_hd; i++) {
        unsigned int first_minor = i << dev->minor_shift;
        current_minor = first_minor + 1;
        check_partition(dev, MKDEV(dev->major, first_minor));
    }
#endif

}
