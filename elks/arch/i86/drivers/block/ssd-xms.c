/*
 * ELKS /dev/ssd XMS ramdisk subdriver
 *
 * Mar 2025 Greg Haerr
 *
 * Use ramdisk utility to allocate from XMS memory:
 *      ramdisk /dev/ssd make 3072      # 3M ramdisk
 *      mkfs /dev/ssd 3072
 *      fsck -lvf /dev/ssd
 *      mount /dev/ssd /mnt
 *      df /dev/ssd
 *      umount /dev/ssd
 */
#include <linuxmt/config.h>
#include <linuxmt/rd.h>
#include <linuxmt/major.h>
#include <linuxmt/fs.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>
#include "ssd.h"

/* current implementation requires no other XMS allocations other than XMS buffers */
static ramdesc_t     xms_ram_base;      /* ramdisk XMS memory start address */
static unsigned  int xms_ram_size;      /* ramdisk size in Kbytes */

/* initialize SSD device */
sector_t ssddev_init(void)
{
    return 0;
}

int ssddev_ioctl(struct inode *inode, struct file *file,
                        unsigned int cmd, unsigned int arg)
{
    if (!suser())
        return -EPERM;

    switch (cmd) {
    case RDCREATE:
        debug_blk("SSD: ioctl make %d\n", arg);
        if (xms_ram_size)
            return -EBUSY;
        xms_ram_base = xms_alloc(arg);      /* in Kbytes */
        if (!xms_ram_base)
            return -ENOMEM;
        xms_ram_size = arg;
        ssd_num_sects = arg << 1;

        /* clear XMS only if using unreal mode as xms_fmemset not supported w/INT 15 */
        if (xms_enabled == XMS_UNREAL) {
            for (sector_t sector = 0; sector < ssd_num_sects; sector++)
                xms_fmemset(0, xms_ram_base + (sector << 9), 0, SD_FIXED_SECTOR_SIZE);
        }
        ssd_initialized = 1;
        return 0;

    case RDDESTROY:
        debug_blk("SSD: ioctl kill\n");
        if (xms_ram_size) {
            invalidate_inodes(inode->i_rdev);
            invalidate_buffers(inode->i_rdev);
            xms_alloc_ptr -= xms_ram_size;      /* NOTE: no xms_free yet */
            xms_ram_size = 0;
            ssd_initialized = 0;
            return 0;
        }
        return -ENXIO;          /* return separate error if ramdisk not inited */
    }
    return -EINVAL;
}

/* write one sector to SSD */
int ssddev_write(sector_t start, char *buf, ramdesc_t seg)
{
    xms_fmemcpyw(0, xms_ram_base + (start << 9), buf, seg, SD_FIXED_SECTOR_SIZE/2);
    return 1;   /* # sectors written */
}

/* read one sector from SSD */
int ssddev_read(sector_t start, char *buf, ramdesc_t seg)
{
    xms_fmemcpyw(buf, seg, 0, xms_ram_base + (start << 9), SD_FIXED_SECTOR_SIZE/2);
    return 1;   /* # sectors read */
}
