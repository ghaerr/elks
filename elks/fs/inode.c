/*
 *  linux/fs/inode.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/stat.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/wait.h>
#include <linuxmt/debug.h>

#include <arch/system.h>

#ifdef BLOAT_FS
int event = 0;
#endif

static struct inode inode_block[NR_INODE];
static struct inode *first_inode;
static struct wait_queue inode_wait;
static int nr_inodes = 0;
static int nr_free_inodes = 0;

static void insert_inode_free(REGOPT struct inode *inode)
{
    register struct inode *in;
    in = inode->i_next = first_inode;
    inode->i_prev = first_inode->i_prev;
    in->i_prev = inode;
    inode->i_prev->i_next = inode;
    first_inode = inode;
}

static void remove_inode_free(REGOPT struct inode *inode)
{
    register struct inode *in;
    if (first_inode == inode)
	first_inode = first_inode->i_next;
    if ((in = inode->i_next))
	in->i_prev = inode->i_prev;
    if (inode->i_prev)
	inode->i_prev->i_next = inode->i_next;
    inode->i_next = inode->i_prev = NULL;
}

static void put_last_free(REGOPT struct inode *inode)
{
    remove_inode_free(inode);
    if (first_inode) {
	inode->i_prev = first_inode->i_prev;
	inode->i_prev->i_next = inode;
    } else
	inode->i_prev = NULL;
    inode->i_next = first_inode;
    inode->i_next->i_prev = inode;
}

static void setup_inodes(void)
{
    REGOPT struct inode *inode;
    int i = NR_INODE;

    inode = inode_block;

    nr_inodes = i;
    nr_free_inodes = i;

    for (; i; i--)
	insert_inode_free(inode++);
}

void inode_init(void)
{
    first_inode = NULL;
    setup_inodes();
}

/*
 * The "new" scheduling primitives (new as of 0.97 or so) allow this to
 * be done without disabling interrupts (other than in the actual queue
 * updating things: only a couple of 386 instructions). This should be
 * much better for interrupt latency.
 */

static void wait_on_inode(REGOPT struct inode *inode)
{
    if (!inode->i_lock)
	return;

    wait_set(&inode->i_wait);
  repeat:
    current->state = TASK_UNINTERRUPTIBLE;
    if (inode->i_lock) {
	schedule();
	goto repeat;
    }
    wait_clear(&inode->i_wait);
    current->state = TASK_RUNNING;
}

static void lock_inode(REGOPT struct inode *inode)
{
    wait_on_inode(inode);
    inode->i_lock = 1;
}

static void unlock_inode(REGOPT struct inode *inode)
{
    inode->i_lock = 0;
    wake_up(&inode->i_wait);
}

/*
 * Note that we don't have to care about wait queues unlike Linux proper
 * because they are not in the object as such
 */

void clear_inode(REGOPT struct inode *inode)
{
    wait_on_inode(inode);
    remove_inode_free(inode);
    if (inode->i_count)
	nr_free_inodes++;
    memset(inode, 0, sizeof(*inode));
    insert_inode_free(inode);
}

int fs_may_mount(kdev_t dev)
{
    REGOPT struct inode *inode, *next;
    int i;

    next = first_inode;
    for (i = nr_inodes; i > 0; i--) {
	inode = next;
	next = inode->i_next;	/* clear_inode() changes the queues.. */
	if (inode->i_dev != dev)
	    continue;
	if (inode->i_count || inode->i_dirt || inode->i_lock)
	    return 0;
	clear_inode(inode);
    }
    return 1;
}

int fs_may_umount(kdev_t dev, REGOPT struct inode *mount_rooti)
{
    REGOPT struct inode *inode;
    int i;

    inode = first_inode;
    for (i = 0; i < nr_inodes; i++, inode = inode->i_next) {
	if (inode->i_dev != dev || !inode->i_count)
	    continue;
	if (inode == mount_rooti && inode->i_count == 1)
	    continue;
	return 0;
    }
    return 1;
}

int fs_may_remount_ro(kdev_t dev)
{
    REGOPT struct file *file;
    register struct inode *inode;
    int i;

    /* Check that no files are currently opened for writing. */
    for (file = file_array, i = 0; i < nr_files; i++, file++) {
	inode = file->f_inode;
	if (!file->f_count || !inode || inode->i_dev != dev)
	    continue;
	if (S_ISREG(inode->i_mode) && (file->f_mode & 2))
	    return 0;
    }
    return 1;
}

static void write_inode(REGOPT struct inode *inode)
{
    register struct super_block *sb = inode->i_sb;
    if (!inode->i_dirt)
	return;
    wait_on_inode(inode);
    if (!inode->i_dirt)
	return;
    if (!sb || !sb->s_op || !sb->s_op->write_inode) {
	inode->i_dirt = 0;
	return;
    }
    inode->i_lock = 1;
    sb->s_op->write_inode(inode);
    unlock_inode(inode);
}

static void read_inode(REGOPT struct inode *inode)
{
    register struct super_block *sb = inode->i_sb;
    register struct super_operations *sop = sb->s_op;

    lock_inode(inode);
    if (sb && sop && sop->read_inode)
	sop->read_inode(inode);
    unlock_inode(inode);
}

/* POSIX UID/GID verification for setting inode attributes */
#if USE_NOTIFY_CHANGE
static int inode_change_ok(REGOPT struct inode *inode,
			   REGOPT struct iattr *attr)
{
    /*
     *      If force is set do it anyway.
     */

    if (attr->ia_valid & ATTR_FORCE)
	return 0;

    /* Make sure a caller can chown */
    if ((attr->ia_valid & ATTR_UID) &&
	(current->euid != inode->i_uid ||
	 attr->ia_uid != inode->i_uid) && !suser()) {
	return -EPERM;
    }

    /* Make sure caller can chgrp */
    if ((attr->ia_valid & ATTR_GID) &&
	(!in_group_p(attr->ia_gid) && attr->ia_gid != inode->i_gid) &&
	!suser())return -EPERM;

    /* Make sure a caller can chmod */
    if (attr->ia_valid & ATTR_MODE) {
	if ((current->euid != inode->i_uid) && !suser())
	    return -EPERM;
	/* Also check the setgid bit! */
	if (!suser()
	    && !in_group_p((attr->ia_valid & ATTR_GID) ? attr->ia_gid :
			   inode->i_gid)) attr->ia_mode &= ~S_ISGID;
    }

    /* Check for setting the inode time */
    if ((attr->ia_valid & (ATTR_ATIME_SET | ATTR_MTIME_SET)) &&
	((current->euid != inode->i_uid) && !suser()))
	return -EPERM;

    return 0;
}

/*
 * Set the appropriate attributes from an attribute structure into
 * the inode structure.
 */

static void inode_setattr(REGOPT struct inode *inode,
			  REGOPT struct iattr *attr)
{
    if (attr->ia_valid & ATTR_UID)
	inode->i_uid = attr->ia_uid;
    if (attr->ia_valid & ATTR_GID)
	inode->i_gid = attr->ia_gid;
    if (attr->ia_valid & ATTR_SIZE)
	inode->i_size = attr->ia_size;
    if (attr->ia_valid & ATTR_MTIME)
	inode->i_mtime = attr->ia_mtime;
    if (attr->ia_valid & ATTR_ATIME)
	inode->i_atime = attr->ia_atime;
    if (attr->ia_valid & ATTR_CTIME)
	inode->i_ctime = attr->ia_ctime;
    if (attr->ia_valid & ATTR_MODE) {
	inode->i_mode = attr->ia_mode;
	if (!suser() && !in_group_p(inode->i_gid))
	    inode->i_mode &= ~S_ISGID;
    }
    inode->i_dirt = 1;
}

/*
 * notify_change is called for inode-changing operations such as
 * chown, chmod, utime, and truncate.  It is guaranteed (unlike
 * write_inode) to be called from the context of the user requesting
 * the change.
 */

int notify_change(REGOPT struct inode *inode, REGOPT struct iattr *attr)
{
    int retval;

    attr->ia_ctime = CURRENT_TIME;
    if (attr->ia_valid & (ATTR_ATIME | ATTR_MTIME)) {
	if (!(attr->ia_valid & ATTR_ATIME_SET))
	    attr->ia_atime = attr->ia_ctime;
	if (!(attr->ia_valid & ATTR_MTIME_SET))
	    attr->ia_mtime = attr->ia_ctime;
    }
#ifdef BLOAT_FS
    if (inode->i_sb && inode->i_sb->s_op && inode->i_sb->s_op->notify_change)
	return inode->i_sb->s_op->notify_change(inode, attr);
#endif

    if ((retval = inode_change_ok(inode, attr)) != 0)
	return retval;

    inode_setattr(inode, attr);
    return 0;
}
#endif

void invalidate_inodes(kdev_t dev)
{
    REGOPT struct inode *inode, *next;
    int i;

    next = first_inode;
    for (i = nr_inodes; i > 0; i--) {
	inode = next;
	next = inode->i_next;	/* clear_inode() changes the queues.. */
	if (inode->i_dev != dev)
	    continue;
	if (inode->i_count || inode->i_dirt || inode->i_lock) {
	    printk("VFS: inode busy on removed device %s\n", kdevname(dev));
	    continue;
	}
	clear_inode(inode);
    }
}

void sync_inodes(kdev_t dev)
{
    int i;
    REGOPT struct inode *inode;

    inode = first_inode;
    for (i = 0; i < nr_inodes * 2; i++, inode = inode->i_next) {
	if (dev && inode->i_dev != dev)
	    continue;
	wait_on_inode(inode);
	if (inode->i_dirt)
	    write_inode(inode);
    }
}

void iput(REGOPT struct inode *inode)
{
    if (inode) {
	wait_on_inode(inode);
	if (!inode->i_count) {
	    printk("VFS: iput: trying to free free inode\n");
	    printk("VFS: device %s, inode %lu, mode=0%07o\n",
		   kdevname(inode->i_rdev), inode->i_ino, inode->i_mode);
	    return;
	}
#ifdef NOT_YET
	if (inode->i_pipe)
	    wake_up_interruptible(&PIPE_WAIT(*inode));
#endif
      repeat:
	if (inode->i_count > 1) {
	    inode->i_count--;
	    return;
	}

	wake_up(&inode_wait);
#ifdef NOT_YET
	if (inode->i_pipe) {
	    /* Free up any memory allocated to the pipe */
	}
#endif

	if (inode->i_sb) {
	    struct super_operations *sop = inode->i_sb->s_op;
	    if (sop && sop->put_inode) {
		sop->put_inode(inode);
		if (!inode->i_nlink)
		    return;
	    }
	}

	if (inode->i_dirt) {
	    write_inode(inode);	/* we can sleep - so do again */
	    wait_on_inode(inode);
	    goto repeat;
	}
	inode->i_count--;
	nr_free_inodes++;
    }

    return;
}

static void list_inode_status(void)
{
    int i;

    for (i = 0; i < nr_inodes; i++)
	printk("[#%u: c=%u d=%x nr=%lu]", i, inode_block[i].i_count,
	       inode_block[i].i_dev, inode_block[i].i_ino);
}

struct inode *get_empty_inode(void)
{
    static ino_t ino = 0;
    REGOPT struct inode *inode, *best;
    int i;

  repeat:
    inode = first_inode;
    best = 0;
    for (inode = inode_block, i = 0; i < nr_inodes; inode++, i++) {
	if (!inode->i_count && !inode->i_lock && !inode->i_dirt) {
	    best = inode;
	    break;
	}
    }
    if (!best) {
	printk("VFS: No free inodes - contact somebody other than Linus\n");
	list_inode_status();
	sleep_on(&inode_wait);
	goto repeat;
    }
/* Here we are doing the same checks again. There cannot be a significant *
 * race condition here - no time has passed */
#if 0
    if (inode->i_lock) {
	wait_on_inode(inode);
	goto repeat;
    }
    if (inode->i_dirt) {
	write_inode(inode);
	goto repeat;
    }
    if (inode->i_count)
	goto repeat;
#endif
    clear_inode(inode);
    inode->i_count = 1;
    inode->i_nlink = 1;
#ifdef BLOAT_FS
    inode->i_version = ++event;
#endif
    inode->i_sem = 0;
    inode->i_ino = ++ino;
    inode->i_dev = 0;
    nr_free_inodes--;
    if (nr_free_inodes < 0) {
	printk("VFS: get_empty_inode: bad free inode count.\n");
	nr_free_inodes = 0;
    }
    return inode;
}

#if CONFIG_PIPE
struct inode *get_pipe_inode(void)
{
    REGOPT struct inode *inode;
    extern struct inode_operations pipe_inode_operations;

    if ((inode = get_empty_inode())) {
	if (!(PIPE_BASE(*inode) = get_pipe_mem())) {
	    iput(inode);
	    return NULL;
	}
	inode->i_op = &pipe_inode_operations;
	inode->i_count = 2;	/* sum of readers/writers */
	PIPE_START(*inode) = PIPE_LEN(*inode) = 0;
	PIPE_RD_OPENERS(*inode) = PIPE_WR_OPENERS(*inode) = 0;
	PIPE_READERS(*inode) = PIPE_WRITERS(*inode) = 1;
	PIPE_LOCK(*inode) = 0;
	inode->i_pipe = 1;
	inode->i_mode |= S_IFIFO | S_IRUSR | S_IWUSR;
	inode->i_uid = current->euid;
	inode->i_gid = current->egid;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
#if 0
	inode->i_blksize = PAGE_SIZE;
#endif
    }
    return inode;
}
#endif

struct inode *__iget(REGOPT struct super_block *sb,
		     ino_t inr /*,int crossmntp */ )
{
    int i;
    REGOPT struct inode *inode;
    REGOPT struct inode *empty = NULL;

    printd_iget3("iget called(%x, %d, %d)\n", sb, inr, 0 /* crossmntp */ );
    if (!sb)
	panic("VFS: iget with sb==NULL");
  repeat:
    inode = inode_block;
    for (i = NR_INODE; i; i--, inode++) {
	if (inode->i_dev == sb->s_dev && inode->i_ino == inr) {
	    goto found_it;
	}
    }

    if (!empty) {
	printd_iget("iget: getting an empty inode...\n");
	empty = get_empty_inode();
	printd_iget1("iget: got one... (%x)!\n", empty);
	if (empty)
	    goto repeat;
	return (NULL);
    }
    inode = empty;
    inode->i_sb = sb;
    inode->i_dev = sb->s_dev;
    inode->i_ino = inr;
    inode->i_flags = ((unsigned short int) sb->s_flags);
    put_last_free(inode);
    printd_iget("iget: Reading inode\n");
    read_inode(inode);
    printd_iget("iget: Read it\n");
    goto return_it;

  found_it:
    if (!inode->i_count)
	nr_free_inodes--;
    inode->i_count++;
    wait_on_inode(inode);
    if (inode->i_dev != sb->s_dev || inode->i_ino != inr) {
	printk("Whee.. inode changed from under us. Tell _.\n");
	iput(inode);
	goto repeat;
    }
    if ( /* crossmntp && */ inode->i_mount) {
	struct inode *tmp = inode->i_mount;
	tmp->i_count++;
	iput(inode);
	inode = tmp;
	wait_on_inode(inode);
    }
    if (empty)
	iput(empty);

  return_it:
    return inode;
}
