#ifndef _SSD_H
#define _SSD_H

/* if sector size not 512, must implement IOCTL_BLK_GET_SECTOR_SIZE */
#define SD_FIXED_SECTOR_SIZE 512

sector_t ssddev_init(void);
int ssddev_ioctl(struct inode *inode, struct file *file,
            unsigned int cmd, unsigned int arg);
int ssddev_write(sector_t start, char *buf, ramdesc_t seg);
int ssddev_read(sector_t start, char *buf, ramdesc_t seg);

extern char ssd_initialized;

#endif /* !_SSD_H */
