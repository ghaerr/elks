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
#include <linuxmt/locks.h>
#include <linuxmt/mm.h>
#include <linuxmt/fs.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/debug.h>

#include <arch/system.h>
#include <arch/segment.h>
#include <arch/bitops.h>
 
extern struct file_operations * get_blkfops();
extern struct file_operations * get_chrfops();

extern int root_mountflags;

struct super_block super_blocks[NR_SUPER];

static int do_remount_sb();

/* this is initialized in init/main.c */
kdev_t ROOT_DEV = CONFIG_ROOTDEV;

/*
 *	Supported File System List
 */

#ifdef CONFIG_MINIX_FS
extern struct file_system_type minix_fs_type;
#endif
#ifdef CONFIG_ROMFS_FS
extern struct file_system_type romfs_fs_type;
#endif
#ifdef CONFIG_ELKSFS_FS
extern struct file_system_type elksfs_fs_type;
#endif

static struct file_system_type * file_systems[] = 
{ 
#ifdef CONFIG_MINIX_FS
	&minix_fs_type,
#endif
#ifdef CONFIG_ROMFS_FS
	&romfs_fs_type,
#endif
#ifdef CONFIG_ELKSFS_FS
	&elksfs_fs_type,
#endif
	NULL
};

/* Should only be called with a near pointer */
#ifdef CONFIG_FULL_VFS 
struct file_system_type *get_fs_type(name)
char *name;
{
	int ct=0;	
	if (!name)
		return file_systems[0];
	while (file_systems[ct]) {
		if (!strcmp(name,file_systems[ct]->name))
			break;
		ct++;
	}
	return file_systems[ct];
}
#endif

void wait_on_super(sb)
register struct super_block * sb;
{
	struct wait_queue wait;

	if (!sb->s_lock)
		return;
	
	wait.task=current;
	wait.next=NULL;

	add_wait_queue(&sb->s_wait, &wait);
repeat:
	current->state = TASK_UNINTERRUPTIBLE;
	if (sb->s_lock) {
		schedule();
		goto repeat;
	}
	remove_wait_queue(&sb->s_wait, &wait);
	current->state = TASK_RUNNING;
}

void lock_super(sb)
register struct super_block *sb;
{
	wait_on_super(sb);
	sb->s_lock=1;
}

void unlock_super(sb)
register struct super_block *sb;
{
	sb->s_lock = 0;
	wake_up(&sb->s_wait);
}

void sync_supers(dev)
kdev_t dev;
{
	register struct super_block * sb;

	for (sb = super_blocks + 0 ; sb < super_blocks + NR_SUPER ; sb++) 
	{
		if (!sb->s_dev)
			continue;
		if (dev && sb->s_dev != dev)
			continue;
		wait_on_super(sb);
		if (!sb->s_dev || !sb->s_dirt)
			continue;
		if (dev && (dev != sb->s_dev))
			continue;
		if (sb->s_op && sb->s_op->write_super)
			sb->s_op->write_super(sb);
	}
}

static struct super_block * get_super(dev)
kdev_t dev;
{
	register struct super_block * s;

	if (!dev)
		return NULL;
	s = 0+super_blocks;
	while (s < NR_SUPER+super_blocks)
		if (s->s_dev == dev) {
			wait_on_super(s);
			if (s->s_dev == dev)
				return s;
			s = 0+super_blocks;
		} else
			s++;
	return NULL;
}

void put_super(dev)
kdev_t dev;
{
	register struct super_block * sb;

	if (dev == ROOT_DEV) {
		panic("put_super: root\n");
		return;
	}
	if (!(sb = get_super(dev)))
		return;
	if (sb->s_covered) {
		printk("VFS: Mounted device %s - tssk, tssk\n",
		       kdevname(dev));
		return;
	}
	if (sb->s_op && sb->s_op->put_super)
		sb->s_op->put_super(sb);
}
#if 0
int sys_ustat(dev,ubuf)
__u16 dev;
struct ustat * ubuf;
{
        register struct super_block *s;
        struct ustat tmp;
        struct statfs sbuf;
        unsigned long old_fs;
        int error;

        s = get_super(to_kdev_t(dev));
        if (s == NULL)
                return -EINVAL;

        if (!(s->s_op->statfs_kern))
                return -ENOSYS;

        s->s_op->statfs_kern(s,&sbuf,sizeof(struct statfs));

        memset(&tmp,0,sizeof(struct ustat));
        tmp.f_tfree = sbuf.f_bfree;
        tmp.f_tinode = sbuf.f_ffree;

        if (error = verified_memcpy_tofs(ubuf,&tmp,sizeof(struct ustat)))
		return error;
        return 0;
}
#endif
static struct super_block * read_super(dev,name,flags,data,silent)
kdev_t dev;
char *name;
int flags;
char * data;
int silent;
{
	register struct super_block * s;
	register struct file_system_type *type;

	if (!dev)
		return NULL;
#ifdef BLOAT_FS
	check_disk_change(dev);
#endif
	s = get_super(dev);
	if (s)
		return s;
#if CONFIG_FULL_VFS
	if (!(type = get_fs_type(name))) {
		printk("VFS: dev %s: get_fs_type(%s) failed\n",
		       kdevname(dev), name);
		return NULL;
	}
#else
	type = file_systems[0];
#endif
	for (s = 0+super_blocks ;; s++) {
		if (s >= NR_SUPER+super_blocks)
			return NULL;
		if (!(s->s_dev))
			break;
	}
	s->s_dev = dev;
	s->s_flags = flags;
	
	if (!type->read_super(s,data, silent)) {
		s->s_dev = 0;
		return NULL;
	}
	s->s_dev = dev;
	s->s_covered = NULL;
#ifdef BLOAT_FS
	s->s_rd_only = 0;
#endif
	s->s_dirt = 0;
	s->s_type = type;
	return s;
}

static int do_umount(dev)
kdev_t dev;
{
	register struct super_block * sb;
	int retval;
	
	if (dev==ROOT_DEV) {
		/* Special case for "unmounting" root.  We just try to remount
		   it readonly, and sync() the device. */
		if (!(sb=get_super(dev)))
			return -ENOENT;
		if (!(sb->s_flags & MS_RDONLY)) {
			fsync_dev(dev);
			retval = do_remount_sb(sb, MS_RDONLY, 0);
			if (retval)
				return retval;
		}
		return 0;
	}
	if (!(sb=get_super(dev)) || !(sb->s_covered))
		return -ENOENT;
	if (!sb->s_covered->i_mount)
		panic("umount: i_mount=NULL\n");
	if (!fs_may_umount(dev, sb->s_mounted))
		return -EBUSY;
	sb->s_covered->i_mount = NULL;
	iput(sb->s_covered);
	sb->s_covered = NULL;
	iput(sb->s_mounted);
	sb->s_mounted = NULL;
	if (sb->s_op && sb->s_op->write_super && sb->s_dirt)
		sb->s_op->write_super(sb);
	put_super(dev);
	return 0;
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

int sys_umount(name)
register char * name;
{
	struct inode * inode;
	kdev_t dev;
	int retval;
	struct inode dummy_inode;
	register struct file_operations * fops;

	if (!suser())
		return -EPERM;
	retval = namei(name,&inode);
	if (retval) {
		retval = lnamei(name,&inode);
		if (retval)
			return retval;
	}
	if (S_ISBLK(inode->i_mode)) {
		dev = inode->i_rdev;
		if (IS_NODEV(inode)) {
			iput(inode);
			return -EACCES;
		}
	} else {
		if (!inode->i_sb || inode != inode->i_sb->s_mounted) {
			iput(inode);
			return -EINVAL;
		}
		dev = inode->i_sb->s_dev;
		iput(inode);
		memset(&dummy_inode, 0, sizeof(dummy_inode));
		dummy_inode.i_rdev = dev;
		inode = &dummy_inode;
	}
	if (MAJOR(dev) >= MAX_BLKDEV) {
		iput(inode);
		return -ENXIO;
	}
	if (!(retval = do_umount(dev)) && dev != ROOT_DEV) {
		fops = get_blkfops(MAJOR(dev));
		if (fops && fops->release)
			fops->release(inode,NULL);
#ifdef NOT_YET
		if (MAJOR(dev) == UNNAMED_MAJOR)
			put_unnamed_dev(dev);
#endif			
	}
	if (inode != &dummy_inode)
		iput(inode);
	if (retval)
		return retval;
	fsync_dev(dev);
	return 0;
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
 
int do_mount(dev,dir,type,flags,data)
kdev_t dev;
char * dir;
char * type;
int flags;
char * data;
{
	struct inode * dir_i;
	register struct super_block * sb;
	int error;

	error = namei(dir,&dir_i, IS_DIR, 0);
	if (error)
		return error;
	if (dir_i->i_count != 1 || dir_i->i_mount) {
		iput(dir_i);
		return -EBUSY;
	}
	if (!fs_may_mount(dev)) {
		iput(dir_i);
		return -EBUSY;
	}
	sb = read_super(dev,type,flags,data,0);
	if (!sb) {
		iput(dir_i);
		return -EINVAL;
	}
	if (sb->s_covered) {
		iput(dir_i);
		return -EBUSY;
	}
	sb->s_covered = dir_i;
	dir_i->i_mount = sb->s_mounted;
	return 0;		/* we don't iput(dir_i) - see umount */
}


/*
 * Alters the mount flags of a mounted file system. Only the mount point
 * is used as a reference - file system type and the device are ignored.
 * FS-specific mount options can't be altered by remounting.
 */

static int do_remount_sb(sb,flags,data)
register struct super_block *sb;
int flags;
char *data;
{
	int retval;
	
	if (!(flags & MS_RDONLY) && sb->s_dev)
		return -EACCES;
		/*flags |= MS_RDONLY;*/
	/* If we are remounting RDONLY, make sure there are no rw files open */
	if ((flags & MS_RDONLY) && !(sb->s_flags & MS_RDONLY))
		if (!fs_may_remount_ro(sb->s_dev))
			return -EBUSY;
	if (sb->s_op && sb->s_op->remount_fs) {
		retval = sb->s_op->remount_fs(sb, &flags, data);
		if (retval)
			return retval;
	}
	sb->s_flags = (sb->s_flags & ~MS_RMT_MASK) |
		(flags & MS_RMT_MASK);
	return 0;
}

#ifdef BLOAT_FS /* Never called */
static int do_remount(dir,flags,data)
char *dir;
int flags;
char *data;
{
	register struct inode *dir_i;
	int retval;

	retval = namei(dir,&dir_i,0,0);
	if (retval)
		return retval;
	if (dir_i != dir_i->i_sb->s_mounted) {
		iput(dir_i);
		return -EINVAL;
	}
	retval = do_remount_sb(dir_i->i_sb, flags, data);
	iput(dir_i);
	return retval;
}
#endif

/*
 * Flags is a 16-bit value that allows up to 16 non-fs dependent flags to
 * be given to the mount() call (ie: read-only, no-dev, no-suid etc).
 */
 
int sys_mount(dev_name, dir_name, type)
char * dev_name;
char * dir_name;
char * type;
{
	register struct file_system_type * fstype;
	struct inode * inode;
	register struct file_operations * fops;
	kdev_t dev;
	int retval;
	char * t;
	unsigned long flags = 0;
	int new_flags = 0;
	char	ltype[256];		/* is enough isn't it? */

	if (!suser())
		return -EPERM;
#ifdef BLOAT_FS /* new_flags is set to zero, so this is never true. */
	if ((new_flags & MS_REMOUNT) ==  MS_REMOUNT) {
		retval = do_remount(dir_name,
				    new_flags & ~MS_REMOUNT, NULL);
		return retval;
	}
#endif
	/*
	 *	FIMXE: copy type to user cleanly or use numeric types ??
	 */
#ifdef CONFIG_FULL_VFS 
	printd_mount("MOUNT: performing type check\n");

	if ((retval = strlen_fromfs(type)) >= 256)
	{
		printd_mount("MOUNT: type size exceeds 256 characters, trunctating\n");
		retval = 255;
	}

	verified_memcpy_fromfs(ltype, type, retval);
	ltype[retval] = '\0';			/* make asciiz again */

	fstype = get_fs_type(ltype);
	if (!fstype)		
		return -ENODEV;
	printd_mount("MOUNT: type check okay\n");
#else
	fstype = file_systems[0];
#endif
	t = fstype->name;
	fops = NULL;
#ifdef BLOAT_FS
	if (fstype->requires_dev) 
	{
#endif
		retval = namei(dev_name,&inode,0,0);
		if (retval)
			return retval;
		printd_mount("MOUNT: made it through namei\n");
		if (!S_ISBLK(inode->i_mode)) {
			iput(inode);
			return -ENOTBLK;
		}
		if (IS_NODEV(inode)) {
			iput(inode);
			return -EACCES;
		}
		dev = inode->i_rdev;
		if (MAJOR(dev) >= MAX_BLKDEV) {
			iput(inode);
			return -ENXIO;
		}
		fops = get_blkfops(MAJOR(dev));
		if (!fops) {
			iput(inode);
			return -ENOTBLK;
		}
		if (fops->open) {
			struct file dummy;	/* allows read-write or read-only flag */
			memset(&dummy, 0, sizeof(dummy));
			dummy.f_inode = inode;
			dummy.f_mode = (new_flags & MS_RDONLY) ? 1 : 3;
			retval = fops->open(inode, &dummy);
			if (retval) {
				iput(inode);
				return retval;
			}
		}

#ifdef BLOAT_FS
	} else {
		printk("unnumbered fs is unsupported.\n");
	}
#endif
	retval = do_mount(dev,dir_name,t,new_flags,NULL);
	if (retval && fops && fops->release)
		fops->release(inode, NULL);
	iput(inode);
	return retval;
}

void mount_root()
{
	register struct file_system_type **fs_type;
	register struct super_block * sb;
	struct inode * inode;
	struct inode d_inode;
	struct file filp;
	int retval;

/* If the line below is removed, the kernel crashes shortly after login *
 * initially giving messages about an invalid block device (0080), and  *
 * just locks up completely. It looks as though the is some data segment*
 * corruption, but I have not been able to find where from              *
 * The crash occurs if the string is less than three chars, or the line *
 * is not there  - AJR 10 May 98                                        */
	printk("BUG\n");
retry_floppy:
/* I don't think this line is needed, the BSS is already zeroed - AJR 9May98*/
/*	memset(super_blocks, 0, sizeof(super_blocks)); */
	memset(&filp, 0, sizeof(filp));
	memset(&d_inode, 0, sizeof(d_inode));
	d_inode.i_rdev = ROOT_DEV;
	filp.f_inode = &d_inode;
	if ( root_mountflags & MS_RDONLY)
		filp.f_mode = 1; /* read only */
	else
		filp.f_mode = 3; /* read write */
		
	retval = blkdev_open(&d_inode, &filp);
	if(retval == -EROFS){
		root_mountflags |= MS_RDONLY;
		filp.f_mode = 1;
		retval = blkdev_open(&d_inode, &filp);
	}
	
	for(fs_type = &file_systems[0];*fs_type;fs_type++)
	{
		struct file_system_type *fp=*fs_type;
		if(retval)
			break;
#ifdef BLOAT_FS
		if (!fp->requires_dev)
			continue;
#endif
		sb = read_super(ROOT_DEV,fp->name,root_mountflags,NULL,1);
		if (sb) {
			inode = sb->s_mounted;
			inode->i_count += 3 ;	/* NOTE! it is logically used 4 times, not 1 */
			sb->s_covered = inode;
			sb->s_flags = root_mountflags;
			current->fs.pwd = inode;
			current->fs.root = inode;
			printk ("VFS: Mounted root (%s filesystem)%s.\n",
				fp->name,
				(sb->s_flags & MS_RDONLY) ? " readonly" : "");
			return;
		}
	}
#ifdef CONFIG_BLK_DEV_BIOS
	if (ROOT_DEV == 0x0380) {
		if(filp.f_op->release)
			filp.f_op->release(&d_inode, &filp);
		else
			printk("Release not defined\n");
		printk("VFS: Insert root floppy and press ENTER\n");
		wait_for_keypress();
		goto retry_floppy;
	}
#endif	

	panic("VFS: Unable to mount root fs on %s\n", kdevname(ROOT_DEV));
}
