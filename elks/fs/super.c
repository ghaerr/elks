/*
 *  linux/fs/super.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/stat.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/mm.h>
#include <linuxmt/fs.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/debug.h>

#include <arch/system.h>
#include <arch/segment.h>
#include <arch/bitops.h>

extern int root_mountflags;

struct super_block super_blocks[NR_SUPER];

static int do_remount_sb(register struct super_block *sb, int flags, char *data);

kdev_t ROOT_DEV;

/* Supported File System List */

#ifdef CONFIG_MINIX_FS
extern struct file_system_type minix_fs_type;
#endif

#ifdef CONFIG_ROMFS_FS
extern struct file_system_type romfs_fs_type;
#endif

#ifdef CONFIG_FS_FAT
extern struct file_system_type msdos_fs_type;
#endif

static struct file_system_type *file_systems[] = {
/* first filesystem is default filesystem for mount w/o -t parm*/
#ifdef CONFIG_ROMFS_FS
	&romfs_fs_type,
#endif
#ifdef CONFIG_MINIX_FS
	&minix_fs_type,
#endif
#ifdef CONFIG_FS_FAT
	&msdos_fs_type,
#endif
	NULL
};
static char *fsname[] = { NULL, "minix", "msdos", "romfs" };

#ifdef CONFIG_FULL_VFS
static struct file_system_type *get_fs_type(int type)
{
    int ct = -1;

    if (type == 0) return file_systems[0];
    while (file_systems[++ct])
	if (file_systems[ct]->type == type) break;
    return file_systems[ct];
}
#endif

void wait_on_super(register struct super_block *sb)
{
    while (sb->s_lock)
	sleep_on(&sb->s_wait);
}

void lock_super(register struct super_block *sb)
{
    wait_on_super(sb);
    sb->s_lock = 1;
}

void unlock_super(register struct super_block *sb)
{
    sb->s_lock = 0;
    wake_up(&sb->s_wait);
}

void sync_supers(kdev_t dev)
{
    register struct super_operations *sop;
    register struct super_block *sb = super_blocks;

    do {
	if ((!sb->s_dev) || (dev && sb->s_dev != dev)) continue;
	wait_on_super(sb);
	if (!sb->s_dev || !sb->s_dirt || (dev && (dev != sb->s_dev))) continue;
	sop = sb->s_op;
	if (sop && sop->write_super) sop->write_super(sb);
    } while (++sb < super_blocks + NR_SUPER);
}

static struct super_block *get_super(kdev_t dev)
{
    register struct super_block *s;

    if (dev) {
      repeat:
	s = super_blocks;
	do {
	    if (s->s_dev == dev) {
		wait_on_super(s);
		if (s->s_dev == dev) return s;
		goto repeat;
	    }
	} while (++s < super_blocks + NR_SUPER);
    }
    return NULL;
}

void put_super(kdev_t dev)
{
    register struct super_block *sb;
    register struct super_operations *sop;

    if (dev == ROOT_DEV)
	panic("put_super: root\n");

    if (!(sb = get_super(dev))) return;
    if (sb->s_covered)
	printk("VFS: Mounted device %s - tssk, tssk\n", kdevname(dev));
    else {
	sop = sb->s_op;
	if (sop && sop->put_super) sop->put_super(sb);
    }
}

#ifdef BLOAT_FS

int sys_ustat(__u16 dev, struct ustat *ubuf)
{
    register struct super_block *s;
    struct ustat tmp;
    struct statfs sbuf;

    s = get_super(to_kdev_t(dev));
    if (s == NULL) return -EINVAL;

    if (!(s->s_op->statfs_kern)) return -ENOSYS;

    s->s_op->statfs_kern(s, &sbuf, sizeof(struct statfs));

    memset(&tmp, 0, sizeof(struct ustat));
    tmp.f_tfree = sbuf.f_bfree;
    tmp.f_tinode = sbuf.f_ffree;

    return verified_memcpy_tofs(ubuf, &tmp, sizeof(struct ustat)))
}

#endif

static struct super_block *read_super(kdev_t dev, int t, int flags,
				      char *data, int silent)
{
    register struct super_block *s;
    register struct file_system_type *type;

    if (!dev) return NULL;
#ifdef BLOAT_FS
    check_disk_change(dev);
#endif
    s = get_super(dev);
    if (s) return s;

#if CONFIG_FULL_VFS
    if (!(type = get_fs_type(t))) {
	printk("VFS: device %s unknown fs type %d\n", kdevname(dev), t);
	return NULL;
    }
#else
    type = file_systems[0];
#endif

    for (s = super_blocks; s->s_dev; s++) {
	if (s >= super_blocks + NR_SUPER) return NULL;
    }
    s->s_dev = dev;
    s->s_flags = (unsigned short int) flags;

    if (!type->read_super(s, data, silent)) {
	s->s_dev = 0;
	return NULL;
    }
    s->s_covered = NULL;
    s->s_dirt = 0;
    s->s_type = type;

#ifdef BLOAT_FS
    s->s_rd_only = 0;
#endif

    return s;
}

static int do_umount(kdev_t dev)
{
    register struct super_block *sb;
    register struct super_operations *sop;
    int retval = -ENOENT;

    if ((sb = get_super(dev))) {
	if (dev == ROOT_DEV) {
	    /* Special case for "unmounting" root.  We just try to remount
	    * it readonly, and sync() the device.
	    */
	    retval = 0;
	    if (!(sb->s_flags & MS_RDONLY)) {
		debug_sup("UMOUNT remount fsync\n");
		fsync_dev(dev);
		retval = do_remount_sb(sb, MS_RDONLY, 0);
		debug_sup("UMOUNT remount returns %d\n", retval);
	    }
	}
	else if (sb->s_covered) {
	    if (!sb->s_covered->i_mount) panic("umount: i_mount=NULL\n");
	    if (!fs_may_umount(dev, sb->s_mounted)) retval = -EBUSY;
	    else {
		retval = 0;
		sb->s_covered->i_mount = NULL;
		iput(sb->s_covered);
		sb->s_covered = NULL;
		iput(sb->s_mounted);
		sb->s_mounted = NULL;
		sop = sb->s_op;
		if (sop && sop->write_super && sb->s_dirt) sop->write_super(sb);
		put_super(dev);
	    }
	}
    }
    return retval;
}

/*
 * Now umount can handle mount points as well as block devices.
 * This is important for filesystems which use unnamed block devices.
 *
 * There is a little kludge here with the dummy_inode.  The current
 * vfs release functions only use the r_dev field in the inode so
 * we give them the info they need without using a real inode.
 * If any other fields are ever needed by any block device release
 * functions, they should be faked here.  -- jrs
 */

int sys_umount(char *name)
{
    struct inode *inode;
    register struct inode *inodep;
    kdev_t dev;
    int retval;

    if (!suser()) return -EPERM;
    retval = namei(name, &inode, 0, 0);
    if (retval) {
	retval = lnamei(name, &inode);
	if (retval) return retval;
    }
    inodep = inode;
    if (S_ISBLK(inodep->i_mode)) {
	if (IS_NODEV(inodep)) {
	    iput(inodep);
	    return -EACCES;
	}
    } else {
	register struct super_block *sb = inodep->i_sb;
	if (!sb || inodep != sb->s_mounted) {
	    iput(inodep);
	    return -EINVAL;
	}
	iput(inodep);
	inodep = new_inode(NULL, S_IFBLK);
	inodep->i_rdev = sb->s_dev;
    }
    dev = inodep->i_rdev;
    if (MAJOR(dev) >= MAX_BLKDEV) {
	iput(inodep);
	return -ENXIO;
    }
    if (!(retval = do_umount(dev)) && dev != ROOT_DEV) {
	register struct file_operations *fops;
	fops = get_blkfops(MAJOR(dev));
	if (fops && fops->release) fops->release(inodep, NULL);

#ifdef NOT_YET
	if (MAJOR(dev) == UNNAMED_MAJOR) put_unnamed_dev(dev);
#endif

    }
    iput(inodep);
    if (!retval) {
	debug_sup("UMOUNT fsync\n");
	fsync_dev(dev);
    }
    return retval;
}

/*
 * do_mount() does the actual mounting after sys_mount has done the ugly
 * parameter parsing. When enough time has gone by, and everything uses the
 * new mount() parameters, sys_mount() can then be cleaned up.
 *
 * We cannot mount a filesystem if it has active, used, or dirty inodes.
 * We also have to flush all inode-data for this device, as the new mount
 * might need new info.
 */

int do_mount(kdev_t dev, char *dir, int type, int flags, char *data)
{
    struct inode *dir_i;
    register struct inode *dirp;
    register struct super_block *sb;
    int error;

    if ((error = namei(dir, &dir_i, IS_DIR, 0))) goto ERROUT;
    dirp = dir_i;
    if ((dirp->i_count != 1 || dirp->i_mount) || (!fs_may_mount(dev)))
 	goto BUSY;
    sb = read_super(dev, type, flags, data, 0);
    if (!sb) {
	error = -EINVAL;
	goto ERROUT1;
    }
    if (sb->s_covered) {
      BUSY:
	error = -EBUSY;
      ERROUT1:
	iput(dirp);
    }
    else {
	sb->s_covered = dirp;
	dirp->i_mount = sb->s_mounted;
	error = 0;		/* we don't iput(dir_i) - see umount */
    }
  ERROUT:
    debug_sup("MOUNT error %d\n", error);
    return error;	
}

/*
 * Alters the mount flags of a mounted file system. Only the mount point
 * is used as a reference - file system type and the device are ignored.
 * FS-specific mount options can't be altered by remounting.
 */

static int do_remount_sb(register struct super_block *sb, int flags, char *data)
{
    int retval;
    register struct super_operations *sop = sb->s_op;

    debug_sup("REMOUNT sb check %d,%d\n", sb->s_flags, flags);
#if 0
    flags |= MS_RDONLY;
#endif

    /* If we are remounting RDONLY, make sure there are no rw files open */
    if ((flags & MS_RDONLY) && !(sb->s_flags & MS_RDONLY))
	if (!fs_may_remount_ro(sb->s_dev)) return -EBUSY;
    if (sop && sop->remount_fs) {
	retval = sop->remount_fs(sb, &flags, data);
	if (retval) {
		debug_sup("REMOUNT fail\n");
		return retval;
	}
    }
    sb->s_flags = (unsigned short int) ((sb->s_flags & ~MS_RMT_MASK) | (flags & MS_RMT_MASK));
    debug_sup("REMOUNT ok\n");
    return 0;
}

static int do_remount(char *dir, int flags, char *data)
{
    struct inode *dir_i;
    int retval;

    if (!(retval = namei(dir, &dir_i, 0, 0))) {
	if (dir_i != dir_i->i_sb->s_mounted) retval = -EINVAL;
	else retval = do_remount_sb(dir_i->i_sb, flags, data);
	iput(dir_i);
    }
    return retval;
}

/*
 * Flags is a 16-bit value that allows up to 16 non-fs dependent flags to
 * be given to the mount() call (ie: read-only, no-dev, no-suid etc).
 */
int sys_mount(char *dev_name, char *dir_name, int type, int flags)
{
    register struct file_system_type *fstype;
    struct inode *inode;
    register struct inode *inodep;
    struct file *filp;
    int retval;

    if (!suser()) return -EPERM;

    if (flags & MS_REMOUNT)
	return do_remount(dir_name, flags & ~MS_REMOUNT, NULL);

#ifdef CONFIG_FULL_VFS
    debug("MOUNT: performing type check\n");

    fstype = get_fs_type(type);
    if (!fstype) return -ENODEV;
    debug("MOUNT: type check okay\n");
#else
    fstype = file_systems[0];
#endif

    filp = NULL;

	retval = namei(dev_name, &inode, 0, 0);
	if (retval) return retval;
	inodep = inode;
	debug("MOUNT: made it through namei\n");
	if (!S_ISBLK(inodep->i_mode))
	    retval = -ENOTBLK;
	else if (IS_NODEV(inodep))
	    retval = -EACCES;
	else if (MAJOR(inodep->i_rdev) >= MAX_BLKDEV)
	    retval = -ENXIO;
	else
	    retval = open_filp((mode_t)((flags & MS_RDONLY) ? 1 : 3), inodep, &filp);
	if (retval) {
	    iput(inodep);
	    return retval;
	}

    retval = do_mount(inodep->i_rdev, dir_name, fstype->type, flags, NULL);
    if (retval && filp)
	close_filp(inodep, filp);
    iput(inodep);

    return retval;
}

void mount_root(void)
{
    struct file_system_type **fs_type;
    struct file_system_type *fp;
    register struct super_block *sb;
    register struct inode *d_inode;
    struct file *filp;
    int retval;

    debug_sup("MOUNT root\n");
    d_inode = new_inode(NULL, S_IFBLK);
    d_inode->i_rdev = ROOT_DEV;
  retry_floppy:
    retval = open_filp(((root_mountflags & MS_RDONLY) ? 0 : 2), d_inode, &filp);
    if (retval == -EROFS) {
	root_mountflags |= MS_RDONLY;
	retval = open_filp(0, d_inode, &filp);
    }
    if (retval) {
	printk("VFS: Unable to open root device %s\n", kdevname(ROOT_DEV));
	halt();
    }

    fs_type = &file_systems[0];
    do {
	fp = *fs_type;

#ifdef BLOAT_FS
	if (!fp->requires_dev) continue;
#endif

	sb = read_super(ROOT_DEV, fp->type, root_mountflags, NULL, 1);
	if (sb) {
	    /* NOTE! it is logically used 4 times, not 1 */
	    sb->s_mounted->i_count += 3;
	    sb->s_covered = sb->s_mounted;
/*	    sb->s_flags = (unsigned short int) root_mountflags;*/
	    current->fs.pwd = current->fs.root = sb->s_mounted;
	    printk("VFS: Mounted root 0x%04x (%s filesystem)%s.\n", ROOT_DEV,
		   fsname[fp->type], (sb->s_flags & MS_RDONLY) ? " readonly" : "");
	    iput(d_inode);
	    filp->f_count = 0;
	    return;
	}
    } while (*(++fs_type) && !retval);

#ifdef CONFIG_BLK_DEV_BIOS
    if (ROOT_DEV == 0x0380) {
	if (!filp->f_op->release)
	    printk("Release not defined\n");
	close_filp(d_inode, filp);
	printk("VFS: Insert root floppy and press ENTER\n");
	wait_for_keypress();
	goto retry_floppy;
    }
#endif

    printk("VFS: Unable to mount root fs on %s\n", kdevname(ROOT_DEV));
    halt();
}
