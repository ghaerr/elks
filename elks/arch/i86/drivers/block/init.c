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

extern void rd_load();
extern int chr_dev_init();
extern int blk_dev_init();

void device_setup(void)
{
    register struct gendisk *p;

    chr_dev_init();

#ifndef CONFIG_NOFS
    blk_dev_init();
#endif

    set_irq();

#ifndef CONFIG_NOFS
    for (p = gendisk_head; p; p = p->next)
	setup_dev(p);
#endif

#ifdef CONFIG_BLK_DEV_RAM
    rd_load();
#endif

}
