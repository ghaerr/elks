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

#include <linuxmt/boot.h>
#include <linuxmt/config.h>
#include <linuxmt/fs.h>
#include <linuxmt/genhd.h>
#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/init.h>
#include <linuxmt/string.h>

#include <arch/system.h>

extern void rd_load();
extern void chr_dev_init();
extern int blk_dev_init();

void device_setup(void)
{
    register struct gendisk *p;

    chr_dev_init();
    blk_dev_init();

    set_irq();

    for (p = gendisk_head; p; p = p->next)
	setup_dev(p);

#ifdef CONFIG_BLK_DEV_RAM
    rd_load();
#else
    /*
     * The bootloader may have passed us a ROOT_DEV which is actually a BIOS
     * drive number.  If so, convert it into a proper <major, minor> block
     * device number.  -- tkchia 20200308
     */
    if ((setupw(0x1f6) & EF_BIOS_DEV_NUM) != 0) {
	printk("device_setup: root device is on BIOS drive number 0x%x\n",
	       (unsigned) ROOT_DEV);

	switch (ROOT_DEV) {
	default:
	    printk("device_setup: unknown BIOS drive, falling back on fd0\n");
	    /* fall thru */
	case 0x00:
	    ROOT_DEV = MKDEV(BIOSHD_MAJOR, 0x80);
	    break;
	case 0x01:
	    ROOT_DEV = MKDEV(BIOSHD_MAJOR, 0xc0);
	    break;
	case 0x80:
	    ROOT_DEV = MKDEV(BIOSHD_MAJOR, 0x00);
	    break;
	case 0x81:
	    ROOT_DEV = MKDEV(BIOSHD_MAJOR, 0x40);
	    break;
	}
    }
#endif

}
