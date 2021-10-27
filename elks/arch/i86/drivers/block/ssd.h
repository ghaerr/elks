#ifndef _SSD_H
#define _SSD_H

sector_t ssddev_init(void);
int ssddev_ioctl(struct inode *inode, struct file *file,
            unsigned int cmd, unsigned int arg);
int ssddev_write_blk(sector_t start, char *buf, seg_t seg);
int ssddev_read_blk(sector_t start, char *buf, seg_t seg);

#endif /* !_SSD_H */
