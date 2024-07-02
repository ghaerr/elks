/*
 *  linux/fs/inode.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/init.h>
#include <linuxmt/stat.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/wait.h>
#include <linuxmt/heap.h>
#include <linuxmt/trace.h>
#include <linuxmt/debug.h>

#include <arch/system.h>

int nr_inode = NR_INODE;
int nr_file = NR_FILE;
static struct inode *inode_block;   /* dynamically allocated */
static struct inode *inode_lru;
static struct inode *inode_llru;
static struct wait_queue inode_wait;

#ifdef CHECK_FREECNTS
static int nr_free_inodes;
#define DCR_COUNT(i) if(!(--i->i_count))nr_free_inodes++
#define INR_COUNT(i) if(!(i->i_count++))nr_free_inodes--
#define CLR_COUNT(i) if(i->i_count)nr_free_inodes++
#define SET_COUNT(i) if(--nr_free_inodes < 0) { panic("get_empty_inode: bad free count"); }
#else
#define DCR_COUNT(i) (i->i_count--)
#define INR_COUNT(i) (i->i_count++)
#define CLR_COUNT(i)
#define SET_COUNT(i)
#endif

static void remove_inode_free(register struct inode *inode)
{
    register struct inode *ino;

    if ((ino = inode->i_next)) {
        if ((ino->i_prev = inode->i_prev))
            inode->i_prev->i_next = ino;
        else
            inode_lru = ino;
    }
    else
        (inode_llru = inode->i_prev)->i_next = NULL;
}

static void put_last_lru(register struct inode *inode)
{
    remove_inode_free(inode);
    inode->i_next = NULL;
    (inode->i_prev = inode_llru)->i_next = inode;
    inode_llru = inode;
}

/*
 * Note that we don't have to care about wait queues unlike Linux proper
 * because they are not in the object as such
 */

void clear_inode(register struct inode *inode) /* and put_first_lru() */
{
    remove_inode_free(inode);
    CLR_COUNT(inode);
    memset(inode, 0, sizeof(struct inode));
    //inode->i_prev = NULL;
    (inode->i_next = inode_lru)->i_prev = inode;
    inode_lru = inode;
}

#if defined(CHECK_FREECNTS) && DEBUG_EVENT
static void list_inode_status(void)
{
    int i = 1;
    int inuse = 0;
    struct inode *inode = inode_llru;

    do {
        if (inode->i_count || inode->i_dev || inode->i_dirt) {
            inode->i_path[sizeof(inode->i_path)-1] = '\0';
            printk("\n#%2d: dev %p inode %5lu cnt %2d %c %06o %s", i, inode->i_dev,
                (unsigned long)inode->i_ino, inode->i_count, inode->i_dirt? 'D':' ',
                inode->i_mode, S_ISSOCK(inode->i_mode)? " [socket]": inode->i_path);
        }
        i++;
        if (inode->i_count) inuse++;
    } while ((inode = inode->i_prev) != NULL);
    printk("\nTotal inodes inuse %d/%d (%d free)\n", inuse, NR_INODE, nr_free_inodes);
}
#endif

void INITPROC inode_init(void)
{
    struct inode *inode;

    inode_block = heap_alloc(nr_inode * sizeof(struct inode),
        HEAP_TAG_INODE|HEAP_TAG_CLEAR);
    if (!inode_block) panic("No inode mem");
    file_array = heap_alloc(nr_file * sizeof(struct file),
        HEAP_TAG_FILE|HEAP_TAG_CLEAR);
    if (!file_array) panic("No file mem");

    inode = inode_block + 1;
    inode_lru = inode_block;
    inode_llru = inode_block;
    do {
        inode->i_next = inode->i_prev = inode;
        put_last_lru(inode);
    } while (++inode < &inode_block[nr_inode]);
#ifdef CHECK_FREECNTS
    nr_free_inodes = nr_inode;
#if DEBUG_EVENT
    debug_setcallback(0, list_inode_status);    /* ^N will generate inode list */
#endif
#endif
}

/*
 * The "new" scheduling primitives (new as of 0.97 or so) allow this to
 * be done without disabling interrupts (other than in the actual queue
 * updating things: only a couple of 386 instructions). This should be
 * much better for interrupt latency.
 */

static void wait_on_inode(register struct inode *inode)
{
    while (inode->i_lock) {
        inode->i_count++;
        sleep_on((struct wait_queue *)inode);
        inode->i_count--;
    }
}

static void lock_inode(register struct inode *inode)
{
    wait_on_inode(inode);
    inode->i_lock = 1;
}

static void unlock_inode(register struct inode *inode)
{
    inode->i_lock = 0;
    wake_up((struct wait_queue *)inode);
}

void invalidate_inodes(kdev_t dev)
{
    register struct inode *prev;
    register struct inode *inode = inode_llru;

    do {
        prev = inode->i_prev;   /* clear_inode() changes the queues.. */
        if (inode->i_dev != dev) continue;
        if (inode->i_count || inode->i_dirt || inode->i_lock)
            printk("VFS: inode busy on removed device %D\n", dev);
        else
            clear_inode(inode);
    } while ((inode = prev) != NULL);
}

static void write_inode(register struct inode *inode)
{
    register struct super_block *sb = inode->i_sb;
    if (inode->i_dirt) {
        wait_on_inode(inode);
        if (inode->i_dirt) {
            if (!sb || !sb->s_op || !sb->s_op->write_inode) {
                inode->i_dirt = 0;
            } else {
                inode->i_lock = 1;
                sb->s_op->write_inode(inode);
                unlock_inode(inode);
            }
        }
    }
}

void sync_inodes(kdev_t dev)
{
    register struct inode *inode = inode_llru;

    do {
        if (dev && inode->i_dev != dev) continue;
        wait_on_inode(inode);
        if (inode->i_dirt) write_inode(inode);
    } while ((inode = inode->i_prev) != NULL);
}

static struct inode *get_empty_inode(void)
{
    register struct inode *inode;

    inode = inode_lru;
    while (inode->i_count || inode->i_dirt || inode->i_lock) {
        if ((inode = inode->i_next) == NULL) {
            printk("VFS: No free inodes\n");
            sleep_on(&inode_wait);
            inode = inode_lru;
        }
    }
    clear_inode(inode);
    put_last_lru(inode);
    inode->i_count = inode->i_nlink = 1;
    inode->i_uid = current->euid;
    SET_COUNT(inode)
    return inode;
}

void iput(register struct inode *inode)
{
    register struct super_operations *sop;

    debug("iput dev %D ino %lu count %d\n",
        inode->i_dev, (unsigned long)inode->i_ino, inode->i_count);
    if (inode) {
        wait_on_inode(inode);
        if (!inode->i_count) {
            printk("iput: trying to free free inode dev %D inode %lu mode 0%06o\n",
                inode->i_rdev, (unsigned long)inode->i_ino, inode->i_mode);
            return;
        }
#if UNUSED
        if ((inode->i_mode & S_IFMT) == S_IFIFO)
            wake_up_interruptible(&PIPE_WAIT(*inode));
#endif
        goto ini_loop;
        do {
            write_inode(inode); /* we can sleep - so do again */
            wait_on_inode(inode);
      ini_loop:
            if (inode->i_count > 1)
                break;

            wake_up(&inode_wait);

            if (inode->i_sb) {
                sop = inode->i_sb->s_op;
                if (sop && sop->put_inode) {
                    sop->put_inode(inode);
                    if (!inode->i_nlink) return;
                }
            }

        } while (inode->i_dirt);
        DCR_COUNT(inode);
#ifdef CHECK_FREECNTS
        if (inode->i_count == 0) {
            inode->i_dev = 0;
            inode->i_ino = 0;
        }
#endif
    }
}

static void set_ops(register struct inode *inode)
{
    static unsigned char tabc[] = {
        0, 1, 2, 0, 0, 0, 3, 0,
        0, 0, 0, 0, 4, 0, 0, 0,
    };
    static struct inode_operations *inop[] = {
        NULL,                           /* Invalid */
        &pipe_inode_operations,         /* FIFO */
        &chrdev_inode_operations,
        &blkdev_inode_operations,
#ifdef CONFIG_SOCKET
        &sock_inode_operations,         /* Socket */
#endif
    };

    inode->i_op = inop[(int)tabc[(inode->i_mode & S_IFMT) >> 12]];
}

static void read_inode(register struct inode *inode)
{
    struct super_block *sb = inode->i_sb;
    register struct super_operations *sop;

    lock_inode(inode);
    if (sb && (sop = sb->s_op) && sop->read_inode) {
        sop->read_inode(inode);
        if (inode->i_op == NULL) set_ops(inode);
    }
    unlock_inode(inode);
}

struct inode *new_inode(register struct inode *dir, mode_t mode)
{
    register struct inode *inode;

    inode = get_empty_inode();  /* get_empty_inode() never returns NULL */
    inode->i_gid =(__u8) current->egid;
    if (dir) {
        inode->i_sb = dir->i_sb;
        inode->i_dev = inode->i_sb->s_dev;
        inode->i_flags = inode->i_sb->s_flags;
        if (dir->i_mode & S_ISGID) {
            inode->i_gid = dir->i_gid;
            if (S_ISDIR(mode)) mode |= S_ISGID;
        }
    }

    if (S_ISLNK(mode)) mode |= 0777;
    else mode &= ~(current->fs.umask & 0777);
    inode->i_mode = mode;
    inode->i_mtime = inode->i_atime = inode->i_ctime = current_time();

    set_ops(inode);
    return inode;
}

struct inode *iget(struct super_block *sb, ino_t inr)
{
    register struct inode *inode;
    register struct inode *n_ino;

    if (!sb) panic("iget sb 0");
    debug("iget dev %p ino %lu\n", sb->s_dev, (unsigned long)inr);

    n_ino = NULL;
    goto start;
    do {
        debug("iget: getting an empty inode...\n");
        n_ino = get_empty_inode();      /* This function may sleep and someone else */
      start:                            /* can create the inode */
        inode = inode_llru;
        do {
            if (inode->i_ino == inr && inode->i_dev == sb->s_dev) goto found_it;
        } while ((inode = inode->i_prev) != NULL);
    } while (n_ino == NULL);
    inode = n_ino;                      /* Inode not found, use the new structure */
    debug("iget: got one...\n");

    inode->i_sb = sb;
    inode->i_dev = sb->s_dev;
    inode->i_flags = sb->s_flags;
    inode->i_ino = inr;
    read_inode(inode);
    goto return_it;

  found_it:
    if (n_ino != NULL) iput(n_ino);
    INR_COUNT(inode);

    /* This will never happen, FIXME remove */
    if (inode->i_dev != sb->s_dev || inode->i_ino != inr) panic("iget");

    if ( /* crossmntp && */ inode->i_mount) {
        n_ino = inode;
        inode = inode->i_mount;
        inode->i_count++;
        iput(n_ino);
    }
    wait_on_inode(inode);
    put_last_lru(inode);
  return_it:
    return inode;
}

int fs_may_mount(kdev_t dev)    /* and invalidate_inodes() */
{
    register struct inode *prev;
    register struct inode *inode = inode_llru;

    do {
        prev = inode->i_prev;   /* clear_inode() changes the queues.. */
        if (inode->i_dev != dev) continue;
        if (inode->i_count || inode->i_dirt || inode->i_lock) return 0;
        clear_inode(inode);
    } while ((inode = prev) != NULL);
    return 1;
}

int fs_may_umount(kdev_t dev, struct inode *mount_rooti)
{
    register struct inode *inode = inode_llru;

    do {
        if (inode->i_dev != dev || !inode->i_count) continue;
        if ((inode != mount_rooti) || (inode->i_count != 1))
            return 0;
    } while ((inode = inode->i_prev) != NULL);
    return 1;
}

int fs_may_remount_ro(kdev_t dev)
{
    register struct file *file = file_array;
    register struct inode *inode;

    /* Check that no files are currently opened for writing. */
    do {
        inode = file->f_inode;
        if (!file->f_count || !inode || inode->i_dev != dev) continue;
        if (S_ISREG(inode->i_mode) && (file->f_mode & 2)) {
                debug_sup("REMOUNT RO fail: open file\n");
                return 0;
        }
    } while (++file < &file_array[nr_file]);
    debug_sup("REMOUNT RO ok\n");
    return 1;
}

/* POSIX UID/GID verification for setting inode attributes */
#if USE_NOTIFY_CHANGE
static int inode_change_ok(register struct inode *inode,
                           register struct iattr *attr)
{
    /* If force is set do it anyway.  */

    if (attr->ia_valid & ATTR_FORCE) return 0;

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
        if ((current->euid != inode->i_uid) && !suser()) return -EPERM;
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

static void inode_setattr(register struct inode *inode,
                          register struct iattr *attr)
{
    if (attr->ia_valid & ATTR_UID) inode->i_uid = attr->ia_uid;
    if (attr->ia_valid & ATTR_GID) inode->i_gid = attr->ia_gid;
    if (attr->ia_valid & ATTR_SIZE) inode->i_size = attr->ia_size;
    if (attr->ia_valid & ATTR_MTIME) inode->i_mtime = attr->ia_mtime;
    if (attr->ia_valid & ATTR_ATIME) inode->i_atime = attr->ia_atime;
    if (attr->ia_valid & ATTR_CTIME) inode->i_ctime = attr->ia_ctime;
    if (attr->ia_valid & ATTR_MODE) {
        inode->i_mode = attr->ia_mode;
        if (!suser() && !in_group_p(inode->i_gid)) inode->i_mode &= ~S_ISGID;
    }
    inode->i_dirt = 1;
}

/*
 * notify_change is called for inode-changing operations such as
 * chown, chmod, utime, and truncate.  It is guaranteed (unlike
 * write_inode) to be called from the context of the user requesting
 * the change.
 */

int notify_change(register struct inode *inode, register struct iattr *attr)
{
    int retval;

    attr->ia_ctime = current_time();
    if (attr->ia_valid & (ATTR_ATIME | ATTR_MTIME)) {
        if (!(attr->ia_valid & ATTR_ATIME_SET)) attr->ia_atime = attr->ia_ctime;
        if (!(attr->ia_valid & ATTR_MTIME_SET)) attr->ia_mtime = attr->ia_ctime;
    }
    if ((retval = inode_change_ok(inode, attr)) != 0) return retval;

    inode_setattr(inode, attr);
    return 0;
}
#endif
