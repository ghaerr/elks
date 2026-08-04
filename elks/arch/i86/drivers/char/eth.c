/*
 * ELKS Ethernet char device driver
 * June 2022 Greg Haerr
 */

#include <linuxmt/config.h>
#include <linuxmt/init.h>
#include <linuxmt/major.h>
#include <linuxmt/errno.h>
#include <linuxmt/fs.h>
#include <linuxmt/netstat.h>
#include <linuxmt/string.h>

struct eth eths[MAX_ETHS];

/* return file_operations pointer from minor number */
static struct file_operations *get_ops(dev_t dev)
{
    unsigned short minor = MINOR(dev);

    if (minor < MAX_ETHS)
        return eths[MINOR(dev)].ops;
    return NULL;
}

static int eth_open(struct inode *inode, struct file *file)
{
    struct file_operations *ops = get_ops(inode->i_rdev);

    if (!ops)
        return -ENODEV;
    return ops->open(inode, file);
}

static void eth_release(struct inode *inode, struct file *file)
{
    struct file_operations *ops = get_ops(inode->i_rdev);

    if (!ops)
        return;
    ops->release(inode, file);
}

static size_t eth_write(struct inode *inode, struct file *file, char *data, size_t len)
{
    struct file_operations *ops = get_ops(inode->i_rdev);

    if (!ops)
        return -ENODEV;
    return ops->write(inode, file, data, len);
}

static size_t eth_read(struct inode *inode, struct file *file, char *data, size_t len)
{
    struct file_operations *ops = get_ops(inode->i_rdev);

    if (!ops)
        return -ENODEV;
    return ops->read(inode, file, data, len);
}

static int eth_ioctl(struct inode *inode, struct file *file, int cmd, char *arg)
{
    struct file_operations *ops = get_ops(inode->i_rdev);

    if (!ops)
        return -ENODEV;
    return ops->ioctl(inode, file, cmd, arg);
}

static int eth_select(struct inode *inode, struct file *file, int sel_type)
{
    struct file_operations *ops = get_ops(inode->i_rdev);

    if (!ops)
        return -ENODEV;
    return ops->select(inode, file, sel_type);
}

static struct file_operations eth_fops = {
    NULL,               /* lseek */
    eth_read,
    eth_write,
    NULL,               /* readdir */
    eth_select,
    eth_ioctl,
    eth_open,
    eth_release
};

void INITPROC eth_init(void)
{
    register_chrdev(ETH_MAJOR, "eth", &eth_fops);

#ifdef CONFIG_ETH_NE2K
    if (ne0_conf.port != -1) ne2k_drv_init();
#endif
#ifdef CONFIG_ETH_WD
    if (wd0_conf.port != -1) wd_drv_init();
#endif
#ifdef CONFIG_ETH_EL3
    if (el3_conf.port != -1) el3_drv_init();
#endif
#ifdef CONFIG_ETH_ULTRA
    if (ul0_conf.port != -1) ultra_drv_init();
#endif
#ifdef CONFIG_ETH_PCNET
    if (le0_conf.port != -1) pcnet_drv_init();
#endif
}
