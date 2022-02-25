/*
 *  linux/fs/minix/inode.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 */

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/minix_fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>
#include <linuxmt/debug.h>

#include <arch/system.h>
#include <arch/segment.h>
#include <arch/bitops.h>

/* Static functions in this file */

static unsigned short map_iblock(register struct inode *,block_t,block_t,int);
static unsigned short map_izone(register struct inode *,block_t,int);
static int minix_set_super_state(struct super_block *sb, int notflags, int newstate);
static void minix_release_bitmaps(struct super_block *sb);
static void minix_read_inode(register struct inode *);
static struct buffer_head *minix_update_inode(register struct inode *);

/* Function definitions */

static void minix_put_inode(register struct inode *inode)
{
    if (!inode->i_nlink) {
	inode->i_size = 0;
	minix_truncate(inode);
	minix_free_inode(inode);
    }
}

/* set superblock flags state, return old state*/
static int minix_set_super_state(struct super_block *sb, int notflags, int newstate)
{
	struct buffer_head *bh;
	struct minix_super_block *ms;
	int oldstate;

	bh = sb->u.minix_sb.s_sbh;
	map_buffer(bh);
	ms = (struct minix_super_block *)bh->b_data;
	oldstate = ms->s_state;
	if (notflags)
		ms->s_state &= notflags;
	else ms->s_state = newstate;
	debug_sup("MINIX set super state %d\n", newstate);
	mark_buffer_dirty(bh);
	unmap_buffer(bh);
	return oldstate;
}

/* release inode and zone bitmap buffers*/
static void minix_release_bitmaps(struct super_block *sb)
{
	int i = 0;

	do {
		brelse(sb->u.minix_sb.s_imap[i]);
	} while (++i < sb->u.minix_sb.s_imap_blocks);
	i = 0;
	do {
		brelse(sb->u.minix_sb.s_zmap[i]);
	} while (++i < sb->u.minix_sb.s_zmap_blocks);
}

void minix_write_super(register struct super_block *sb)
{
	debug_sup("MINIX write super\n");
	if (!(sb->s_flags & MS_RDONLY))
		minix_set_super_state(sb, ~MINIX_VALID_FS, 0);		/* unset fs checked flag, commit superblock*/
	sb->s_dirt = 0;
}

void minix_put_super(register struct super_block *sb)
{
	debug_sup("MINIX put super\n");
	lock_super(sb);
	if (!(sb->s_flags & MS_RDONLY))
		minix_set_super_state(sb, 0, sb->u.minix_sb.s_mount_state);	/* set original fs state*/
	minix_release_bitmaps(sb);
	brelse(sb->u.minix_sb.s_sbh);
	unlock_super(sb);
	sb->s_dev = 0;
}

static struct super_operations minix_sops = {
    minix_read_inode,
    minix_write_inode,
    minix_put_inode,
    minix_put_super,
    minix_write_super,
    minix_remount,
    minix_statfs
};

static void minix_mount_warning(register struct super_block *sb, char *prefix)
{
	if ((sb->u.minix_sb.s_mount_state & (MINIX_VALID_FS|MINIX_ERROR_FS)) != MINIX_VALID_FS)
		printk("MINIX-fs: %smounting %s 0x%x, running fsck is recommended.\n", prefix,
	     !(sb->u.minix_sb.s_mount_state & MINIX_VALID_FS) ?
		 "unchecked file system" : "file system with errors", sb->s_dev);
}


int minix_remount(register struct super_block *sb, int *flags, char *data)
{
	debug_sup("MINIX remount %d,%d\n", sb->s_flags, *flags);
	if ((*flags & MS_RDONLY) == (sb->s_flags & MS_RDONLY)) {
		debug_sup("MINIX remount both rdonly\n");
		return 0;
	}
	if (*flags & MS_RDONLY) {
		/* Mounting a rw partition read-only*/
		debug_sup("MINIX remount RW to RO\n");
		minix_set_super_state(sb, 0, sb->u.minix_sb.s_mount_state);	/* reset original fs state and commit superblock*/
		sb->s_dirt = 0;
	} else {
		/* Mount a partition which is read-only, read-write*/
		debug_sup("MINIX remount RO to RW\n");
		sb->u.minix_sb.s_mount_state = minix_set_super_state(sb, ~MINIX_VALID_FS, 0); /* unset fs checked flag*/
		sb->s_dirt = 1;
		minix_mount_warning(sb, "re");
	}
    return 0;
}

struct super_block *minix_read_super(register struct super_block *s, char *data, int silent)
{
    struct buffer_head *bh;
	struct minix_super_block *ms;
	int i;
    unsigned short block;
    kdev_t dev = s->s_dev;
    char *msgerr;
    static char *err0 = "";
    static char *err1 = "minix: unable to read sb\n";
    static char *err2 = "minix: bad superblock or bitmaps\n";
    static char *err3 = "minix: get root inode failed\n";
    static char *err4 = "minix: inode table too large\n";

    lock_super(s);
	if (!(bh = bread(dev, (block_t) 1))) {
		msgerr = err1;
		goto err_read_super_2;
    }
    map_buffer(bh);

	ms = (struct minix_super_block *) bh->b_data;
	if (ms->s_magic != MINIX_SUPER_MAGIC) {
	    if (!silent)
		printk("VFS: dev %s is not minixfs.\n", kdevname(dev));
	    msgerr = err0;
	    goto err_read_super_1;
	}
	if (ms->s_imap_blocks > MINIX_I_MAP_SLOTS) {
	    msgerr = err4;
	    goto err_read_super_1;
	}

	s->u.minix_sb.s_sbh = bh;
	s->u.minix_sb.s_dirsize = 16;
	s->u.minix_sb.s_namelen = 14;
	s->u.minix_sb.s_mount_state = ms->s_state;
	s->u.minix_sb.s_ninodes = ms->s_ninodes;
	s->u.minix_sb.s_imap_blocks = ms->s_imap_blocks;
	s->u.minix_sb.s_zmap_blocks = ms->s_zmap_blocks;
	s->u.minix_sb.s_firstdatazone = ms->s_firstdatazone;
	s->u.minix_sb.s_log_zone_size = ms->s_log_zone_size;
	s->u.minix_sb.s_max_size = ms->s_max_size;
	s->u.minix_sb.s_nzones = ms->s_nzones;
#ifdef BLOAT_FS
	s->s_magic = ms->s_magic;
#endif

	/*
	 *      FIXME:: We cant keep these in memory on an 8086, need to change
	 *      the code to fetch/release each time we get a block.
	 */
	block = 2;
	i = 0;
	do {
	    if ((s->u.minix_sb.s_imap[i] = bread(dev, (block_t) block)) == NULL)
			break;
	    block++;
	} while (++i < s->u.minix_sb.s_imap_blocks);
	i = 0;
	do {
	    if ((s->u.minix_sb.s_zmap[i] = bread(dev, (block_t) block)) == NULL)
			break;
	    block++;
	} while (++i < s->u.minix_sb.s_zmap_blocks);
//printk("minix-fs: %d buffers used\n", block-2);
	if (block != 2 + s->u.minix_sb.s_imap_blocks + s->u.minix_sb.s_zmap_blocks) {
		minix_release_bitmaps(s);
		msgerr = err2;
	    goto err_read_super_1;
	}

    set_bit(0, s->u.minix_sb.s_imap[0]->b_data);	/* force don't use inode or zone 0 just in case*/
    set_bit(0, s->u.minix_sb.s_zmap[0]->b_data);
    unlock_super(s);
    /* set up enough so that it can read an inode */
    s->s_op = &minix_sops;
    s->s_mounted = iget(s, (ino_t) MINIX_ROOT_INO);
    if (!s->s_mounted) {
		lock_super(s);
		msgerr = err3;
		goto err_read_super_1;
    }
    if (!(s->s_flags & MS_RDONLY)) {
		ms->s_state &= ~MINIX_VALID_FS;
		mark_buffer_dirty(bh);
		s->s_dirt = 1;
		fsync_dev(s->s_dev);	/* force unchecked flag immediately after mount*/
    }

    minix_mount_warning(s, err0);

    unmap_buffer(bh);
    return s;

  err_read_super_1:
    unmap_brelse(bh);
  err_read_super_2:
    unlock_super(s);
    printk(msgerr);
    return NULL;
}

void minix_statfs(struct super_block *s, struct statfs *sf)
{
    sf->f_bsize = BLOCK_SIZE;
    sf->f_blocks = s->u.minix_sb.s_nzones << s->u.minix_sb.s_log_zone_size;
    sf->f_bfree = minix_count_free_blocks(s);
    sf->f_bavail = sf->f_bfree;
    sf->f_files = s->u.minix_sb.s_ninodes;
    sf->f_ffree = minix_count_free_inodes(s);
}

/* Adapted from Linux 0.12's inode.c.  _bmap() is a big function, I know

   Rewritten 2001 by Alan Cox based on newer kernel code + my own plans */

static unsigned short map_izone(register struct inode *inode, block_t block, int create)
{
    register __u16 *i_zone = &(inode->u.minix_i.i_zone[block]);

    if (create && !(*i_zone)) {
	if ((*i_zone = minix_new_block(inode->i_sb))) {
	    inode->i_ctime = CURRENT_TIME;
	    inode->i_dirt = 1;
	}
    }
    return *i_zone;
}

static unsigned short map_iblock(struct inode *inode, block_t i,
				 block_t block, int create)
{
    register struct buffer_head *bh;
    register block_t *b_zone;

    if (!(bh = bread(inode->i_dev, i))) {
	return 0;
    }
    map_buffer(bh);
    b_zone = &(((block_t *) (bh->b_data))[block]);
    if (create && !(*b_zone)) {
	if ((*b_zone = minix_new_block(inode->i_sb))) {
	    mark_buffer_dirty(bh);
	}
    }
    unmap_brelse(bh);
    return *b_zone;
}

unsigned short _minix_bmap(register struct inode *inode, block_t block, int create)
{
    register char *i;

#if 0
/* I do not understand what this bit means, it cannot be this big,
 * it is a short. If this was a long it would make sense. We need to
 * check for overruns in the block num elsewhere.. FIXME
 */
    if (block > (7 + 512 + 512 * 512))
	panic("_minix_bmap: block (%d) >big", block);
#endif

    if (block < 7)
	return map_izone(inode, block, create);
    block -= 7;
    if (block < 512) {
	i = (char *)map_izone(inode, 7, create);
	goto map1;
    }

    /*
     *      Double indirection country. Do two block remaps
     */

    block -= 512;
    i = (char *)map_izone(inode, 8, create);
    if (i != 0) {
	/* Two layer indirection */
	i = (char *)map_iblock(inode, (block_t)i, (block_t) (block >> 9), create);

  map1:
	/*
	 * Do the final indirect block check and read
	 */
	if (i != 0)
	    /* Ok now load the second indirect block */
	    i = (char *)map_iblock(inode, (block_t)i, (block_t) (block & 511), create);
    }
    return (unsigned short)i;
}

struct buffer_head *minix_getblk(register struct inode *inode, block_t block, int create)
{
    unsigned short blknum;

    if (!(blknum = _minix_bmap(inode, block, create)))
	return NULL;
    return getblk(inode->i_dev, (block_t) blknum);
}

struct buffer_head *minix_bread(struct inode *inode,
				unsigned short block, int create)
{
    register struct buffer_head *bh;

    return !(bh = minix_getblk(inode, block, create)) ? NULL : readbuf(bh);
}

/*
 *	Set the ops on a minix inode
 */

void minix_set_ops(register struct inode *inode)
{
    static unsigned char tabc[] = {
	0, 0, 0, 0, 1, 0, 0, 0,
	2, 0, 3, 0, 0, 0, 0, 0,
    };
    static struct inode_operations *inop[] = {
	NULL,				/* Invalid */
	&minix_dir_inode_operations,
	&minix_file_inode_operations,
	&minix_symlink_inode_operations,
    };

    if (inode->i_op == NULL)
	inode->i_op = inop[(int)tabc[(inode->i_mode & S_IFMT) >> 12]];
}

/*
 * The minix V1 function to read an inode into a buffer.
 */

static struct buffer_head *minix_get_inode(register struct inode *inode,
					   struct minix_inode **raw_inode)
{
    register struct buffer_head *bh = NULL;
    unsigned short block;
    unsigned int ino;

    ino = inode->i_ino;
    if (!ino || ino > inode->i_sb->u.minix_sb.s_ninodes) {
	printk("Bad inode number on dev %s: %d is out of range\n",
		kdevname(inode->i_dev), ino);
    }
    else {
	block = inode->i_sb->u.minix_sb.s_imap_blocks + 2 +
	    inode->i_sb->u.minix_sb.s_zmap_blocks + (ino - 1) / MINIX_INODES_PER_BLOCK;
	if (!(bh = bread(inode->i_dev, (block_t) block))) {
	    printk("unable to read i-node block\n");
	}
	else {
	    map_buffer(bh);
	    *raw_inode = ((struct minix_inode *) bh->b_data) +
		(ino - 1) % MINIX_INODES_PER_BLOCK;
	}
    }
    return bh;
}

/*
 * The minix V1 function to read an inode.
 */

static void minix_read_inode(register struct inode *inode)
{
    register struct buffer_head *bh;
    struct minix_inode *raw_inode;

    bh = minix_get_inode(inode, &raw_inode);
    if (bh) {
	memcpy(inode, raw_inode, 14);
	memcpy(inode->u.minix_i.i_zone, raw_inode->i_zone, 18);
	inode->i_ctime = inode->i_atime = inode->i_mtime;

#ifdef BLOAT_FS
	inode->i_blocks = inode->i_blksize = 0;
#endif

	if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode))
	    inode->i_rdev = to_kdev_t(inode->u.minix_i.i_zone[0]);
	unmap_brelse(bh);
	minix_set_ops(inode);
    }
}

/*
 * The minix V1 function to synchronize an inode.
 */

static struct buffer_head *minix_update_inode(register struct inode *inode)
{
    register struct buffer_head *bh = NULL;
    struct minix_inode *raw_inode;

    bh = minix_get_inode(inode, &raw_inode);
    if (bh) {
	memcpy(raw_inode, inode, 14);
	memcpy(raw_inode->i_zone, inode->u.minix_i.i_zone, 18);
	if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode))
	    raw_inode->i_zone[0] = kdev_t_to_nr(inode->i_rdev);
	inode->i_dirt = 0;
	mark_buffer_dirty(bh);
	unmap_buffer(bh);
    }
    return bh;
}

void minix_write_inode(struct inode *inode)
{
    brelse(minix_update_inode(inode));
}

#ifdef BLOAT_FS
int minix_sync_inode(register struct inode *inode)
{
    int err = 0;
    struct buffer_head *bh;

    bh = minix_update_inode(inode);
    if (bh && buffer_dirty(bh)) {
	ll_rw_blk(WRITE, bh);
	wait_on_buffer(bh);
	if (!buffer_uptodate(bh)) {
	    printk("IO error syncing minix inode [%s:%08lx]\n",
		   kdevname(inode->i_dev), inode->i_ino);
	    err = -1;
	}
    } else if (!bh)
	err = -1;
    brelse(bh);
    return err;
}
#endif

struct file_system_type minix_fs_type = {
    minix_read_super,
	FST_MINIX
};

int init_minix_fs(void)
{
    return 1;
    /*register_filesystem(&minix_fs_type); */
}
