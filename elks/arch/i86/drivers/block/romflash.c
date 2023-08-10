#include <linuxmt/config.h>
#include <linuxmt/major.h>
#include <linuxmt/fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/init.h>

static int romflash_open (struct inode * i, struct file * f)
{
	i->i_size = 0x10000;  // 8086 address space
	return 0;
}


static struct file_operations romflash_fops =
{
	NULL,           /* lseek */
	NULL,           /* read */
	NULL,           /* write */
	NULL,           /* readdir */
	NULL,           /* select */
	NULL,           /* ioctl */
	romflash_open,  /* open */
	NULL            /* release */
};


void romflash_init (void)
{
	if (register_blkdev (ROMFLASH_MAJOR, "rom", &romflash_fops))
		printk ("Unable to register major %u for rom-flash\n", ROMFLASH_MAJOR);
}
