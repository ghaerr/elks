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
#include <linuxmt/boot.h>
#include <linuxmt/fs.h>
#include <linuxmt/genhd.h>
#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/init.h>
#include <linuxmt/string.h>
#include <linuxmt/memory.h>
#include <linuxmt/devnum.h>

#include <arch/irq.h>
#include <arch/system.h>
#include <arch/segment.h>

int boot_rootdev;       /* set by /bootopts options if configured*/

void INITPROC device_init(void)
{
    chr_dev_init();

    set_irq();          /* interrupts enabled for possible disk I/O or timers */

    blk_dev_init();

#if defined(CONFIG_BLK_DEV_BFD) || defined(CONFIG_BLK_DEV_BHD) || \
    defined(CONFIG_BLK_DEV_FD) || defined(CONFIG_BLK_DEV_ATA_CF)
    /*
     * The bootloader may have passed us a ROOT_DEV which is actually a BIOS
     * drive number.  If so, convert it into a proper <major, minor> block
     * device number.  -- tkchia 20200308
     *
     * If linked in, the BIOS FD/HD driver(s) take precedence over
     * the DF or ATA drivers for determine boot drive and partition.
     */
    if (!boot_rootdev && (SETUP_ELKS_FLAGS & EF_BIOS_DEV_NUM) != 0) {
        dev_t rootdev = 0;
#if defined(CONFIG_BLK_DEV_ATA_CF) && !defined(CONFIG_BLK_DEV_BHD)
        if (ROOT_DEV & 0x80) {
            int minor = (ROOT_DEV & 1) << 3;
            rootdev = MKDEV(ATHD_MAJOR, minor + boot_partition);
        }
#endif
#if defined(CONFIG_BLK_DEV_FD) && !defined(CONFIG_BLK_DEV_BFD)
        if (ROOT_DEV == 0) rootdev = DEV_DF0;
        if (ROOT_DEV == 1) rootdev = DEV_DF1;
#endif
#if defined(CONFIG_BLK_DEV_BFD) || defined(CONFIG_BLK_DEV_BHD)
        if (!rootdev)
            rootdev = bios_conv_bios_drive(ROOT_DEV);
#endif
        printk("boot: BIOS drive %x, root device %E (%D)\n", ROOT_DEV, rootdev, rootdev);
        ROOT_DEV = (kdev_t)rootdev;
    }
#endif
}
