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

void minix_put_inode(inode)
register struct inode *inode;
{
	if (inode->i_nlink)
		return;
	inode->i_size = 0;
#ifndef CONFIG_FS_RO
	minix_truncate(inode);
#endif
	minix_free_inode(inode);
}

#ifndef CONFIG_FS_RO
static void minix_commit_super (sb)
register struct super_block * sb;
/*struct minix_super_block * ms; */
{
	mark_buffer_dirty(sb->u.minix_sb.s_sbh, 1);
	sb->s_dirt = 0;
}

void minix_write_super (sb)
register struct super_block * sb;
{
	register struct minix_super_block * ms;
	if (!(sb->s_flags & MS_RDONLY)) {
		ms = sb->u.minix_sb.s_ms;

		if (ms->s_state & MINIX_VALID_FS)
			ms->s_state &= ~MINIX_VALID_FS;
		minix_commit_super (sb);
	}
	sb->s_dirt = 0;
}
#endif /*CONFIG_FS_RO */


void minix_put_super(sb)
register struct super_block *sb;
{
	int i;

	lock_super(sb);
#ifndef CONFIG_FS_RO
	if (!(sb->s_flags & MS_RDONLY)) {
		sb->u.minix_sb.s_ms->s_state = sb->u.minix_sb.s_mount_state;
		mark_buffer_dirty(sb->u.minix_sb.s_sbh, 1);
	}
#endif
	sb->s_dev = 0;
	for(i = 0 ; i < MINIX_I_MAP_SLOTS ; i++)
		brelse(sb->u.minix_sb.s_imap[i]);
	for(i = 0 ; i < MINIX_Z_MAP_SLOTS ; i++)
		brelse(sb->u.minix_sb.s_zmap[i]);
	brelse (sb->u.minix_sb.s_sbh);
	unlock_super(sb);
	return;
}

static struct super_operations minix_sops = { 
	minix_read_inode,
#ifdef BLOAT_FS
	NULL,
#endif
#ifdef CONFIG_FS_RO
	NULL,
#else
	minix_write_inode,
#endif
	minix_put_inode,
	minix_put_super,
#ifdef CONFIG_FS_RO
	NULL,
#else
	minix_write_super,
#endif
#ifdef BLOAT_FS
	minix_statfs,
#endif
	minix_remount
};

int minix_remount (sb,flags,data)
register struct super_block * sb;
int * flags;
char * data;
{
	register struct minix_super_block * ms;

	ms = sb->u.minix_sb.s_ms;

#ifdef CONFIG_FS_RO
	if (!(*flags & MS_RDONLY))
		return -EROFS;
#else
	if ((*flags & MS_RDONLY) == (sb->s_flags & MS_RDONLY))
		return 0;
	if (*flags & MS_RDONLY) {
		if (ms->s_state & MINIX_VALID_FS ||
		    !(sb->u.minix_sb.s_mount_state & MINIX_VALID_FS))
			return 0;
		/* Mounting a rw partition read-only. */
		ms->s_state = sb->u.minix_sb.s_mount_state;
		mark_buffer_dirty(sb->u.minix_sb.s_sbh, 1);
		sb->s_dirt = 1;
		minix_commit_super (sb);
	}
	else {
	  	/* Mount a partition which is read-only, read-write. */
		sb->u.minix_sb.s_mount_state = ms->s_state;
		ms->s_state &= ~MINIX_VALID_FS;
		mark_buffer_dirty(sb->u.minix_sb.s_sbh, 1);
		sb->s_dirt = 1;

		if (!(sb->u.minix_sb.s_mount_state & MINIX_VALID_FS))
			printk ("MINIX-fs warning: remounting unchecked fs, running fsck is recommended.\n");
		else if ((sb->u.minix_sb.s_mount_state & MINIX_ERROR_FS))
			printk ("MINIX-fs warning: remounting fs with errors, running fsck is recommended.\n");
	}
#endif
	return 0;
}

struct super_block *minix_read_super(s,data,silent)
register struct super_block *s;
char *data;
int silent;
{
	struct buffer_head *bh;
	int i;
	unsigned short block;
	kdev_t dev = s->s_dev;

	if (32 != sizeof (struct minix_inode))
		panic("bad i-node size");
	lock_super(s);
	if (!(bh = bread(dev,1L))) {
		s->s_dev = 0;
		unlock_super(s);
		printk("MINIX-fs: unable to read superblock\n");
		return NULL;
	}
	map_buffer(bh);
	{ /* Localise register variable */
		register struct minix_super_block *ms;
		ms = (struct minix_super_block *) bh->b_data;
		s->u.minix_sb.s_ms = ms;
		s->u.minix_sb.s_sbh = bh;
		s->u.minix_sb.s_mount_state = ms->s_state;
		s->u.minix_sb.s_ninodes = ms->s_ninodes;
		s->u.minix_sb.s_imap_blocks = ms->s_imap_blocks;
		s->u.minix_sb.s_zmap_blocks = ms->s_zmap_blocks;
		s->u.minix_sb.s_firstdatazone = ms->s_firstdatazone;
		s->u.minix_sb.s_log_zone_size = ms->s_log_zone_size;
		s->u.minix_sb.s_max_size = ms->s_max_size;
#ifdef BLOAT_FS
		s->s_magic = ms->s_magic;
#endif
	if (ms->s_magic == MINIX_SUPER_MAGIC) {
		s->u.minix_sb.s_version = MINIX_V1;
		s->u.minix_sb.s_nzones = s->u.minix_sb.s_ms->s_nzones;
		s->u.minix_sb.s_dirsize = 16;
		s->u.minix_sb.s_namelen = 14;
	} else {
		s->s_dev = 0;
		unlock_super(s);
		unmap_brelse(bh);
		if (!silent)
			printk("VFS: Can't find a Minix V1 filesystem on dev %s.\n", kdevname(dev));
		return NULL;
	}
	}
	for (i=0;i < MINIX_I_MAP_SLOTS;i++)
		s->u.minix_sb.s_imap[i] = NULL;
	for (i=0;i < MINIX_Z_MAP_SLOTS;i++)
		s->u.minix_sb.s_zmap[i] = NULL;
	block=2;
	/*
	 *	FIXME:: We cant keep these in memory on an 8086, need to change
	 *	the code to fetch/release each time we get a block.
	 */
	for (i=0 ; i < s->u.minix_sb.s_imap_blocks ; i++)
		if ((s->u.minix_sb.s_imap[i]=bread(dev,(unsigned long)block)) != NULL)
			block++;
		else
			break;
	for (i=0 ; i < s->u.minix_sb.s_zmap_blocks ; i++)
		if ((s->u.minix_sb.s_zmap[i]=bread(dev,(unsigned long)block)) != NULL)
			block++;
		else
			break;
	if (block != 2+s->u.minix_sb.s_imap_blocks+s->u.minix_sb.s_zmap_blocks) {
		for(i=0;i<MINIX_I_MAP_SLOTS;i++)
			brelse(s->u.minix_sb.s_imap[i]);
		for(i=0;i<MINIX_Z_MAP_SLOTS;i++)
			brelse(s->u.minix_sb.s_zmap[i]);
		s->s_dev = 0;
		unlock_super(s);
		unmap_brelse(bh);
		printk("minix: bad superblock or bitmaps\n");
		return NULL;
	}
	set_bit(0,s->u.minix_sb.s_imap[0]->b_data);
	set_bit(0,s->u.minix_sb.s_zmap[0]->b_data);
	unlock_super(s);
	/* set up enough so that it can read an inode */
	s->s_dev = dev;
	s->s_op = &minix_sops;
	s->s_mounted = iget(s, (long) MINIX_ROOT_INO);
	if (!s->s_mounted) {
		s->s_dev = 0;
		unmap_brelse(bh);
		printk("minix: get root inode failed\n");
		return NULL;
	}
	if (!(s->s_flags & MS_RDONLY)) {
		s->u.minix_sb.s_ms->s_state &= ~MINIX_VALID_FS;
		mark_buffer_dirty(bh, 1);
		s->s_dirt = 1;
	}
	if (!(s->u.minix_sb.s_mount_state & MINIX_VALID_FS))
		printk ("MINIX-fs: mounting unchecked file system, running fsck is recommended.\n");
 	else if (s->u.minix_sb.s_mount_state & MINIX_ERROR_FS)
		printk ("MINIX-fs: mounting file system with errors, running fsck is recommended.\n");
	unmap_buffer(bh);
	return s;
}

#ifdef BLOAT_FS
void minix_statfs(sb,buf,bufsiz)
register struct super_block *sb;
struct statfs *buf;
int bufsiz;
{
	struct statfs tmp;

	tmp.f_type = sb->s_magic;
	tmp.f_bsize = BLOCK_SIZE;
	ptmp->f_blocks = (sb->u.minix_sb.s_nzones - sb->u.minix_sb.s_firstdatazone) << sb->u.minix_sb.s_log_zone_size;
	tmp.f_bfree = minix_count_free_blocks(sb);
	tmp.f_bavail = tmp.f_bfree;
	tmp.f_files = sb->u.minix_sb.s_ninodes;
	tmp.f_ffree = minix_count_free_inodes(sb);
	tmp.f_namelen = sb->u.minix_sb.s_namelen;
	memcpy(buf, &tmp, bufsiz);
}
#endif

/* Adapted from Linux 0.12's inode.c.  _bmap() is a long function, I know */

unsigned short _minix_bmap(inode, block, create)
register struct inode * inode;
unsigned short block;
int create;
{
	struct buffer_head * bh;
	register unsigned short *i_zone = inode->i_zone;
	unsigned short i;

/* I do not understand what this bit means, it cannot be this big,
 * it is a short */
#if 0
	if (block > (7+512+512*512))	
		panic("_minix_bmap: block (%d) >big", block);
#endif

	if (block < 7) {
#ifndef CONFIG_FS_RO
		if (create && !i_zone[block]) {
			if (i_zone[block]=minix_new_block(inode->i_sb)) {
				inode->i_ctime = CURRENT_TIME;
				inode->i_dirt = 1;
			}
		}
#endif
		return i_zone[block];
	}
	block -= 7;
	if (block <= 512) {
		if (!i_zone[7]) {
#ifndef CONFIG_FS_RO
			if (create) {
				if (i_zone[7]=minix_new_block(inode->i_sb)) {
					inode->i_dirt = 1;
					inode->i_ctime = CURRENT_TIME;
				}
			} else return 0;
#else
			return 0;
#endif
		}
		printd_mfs1("MFSbmap: About to read indirect block #%d\n", i_zone[7]);
		if (!(bh = bread(inode->i_dev, (unsigned long)i_zone[7]))) {
			printd_mfs("MFSbmap: Bread of zone 7 failed\n");
			return 0;
		}
		map_buffer(bh);
		i = (((unsigned short *) (bh->b_data))[block]);
#ifndef CONFIG_FS_RO
		if (create && !i) {
			if (i=minix_new_block(inode->i_sb)) {
				((unsigned short *) (bh->b_data))[block] = i;
				bh->b_dirty = 1;
			}
		}
#endif
		unmap_brelse(bh);
		printd_mfs1("MFSbmap: Returning #%d\n", i);
		return i;
	}
	printk("MINIX-fs: bmap cannot handle > 519K files yet!\n");
	return 0;
}

struct buffer_head * minix_getblk(inode,block,create)
register struct inode * inode;
unsigned short block;
int create;
{
	struct buffer_head * bh;
	unsigned short blknum;

	blknum = _minix_bmap(inode, block, create);
	printd_mfs2("MINIXfs: file block #%d -> disk block #%d\n", block, blknum);
	if (blknum != 0) {
		bh = getblk(inode->i_dev, (unsigned long)blknum); 
		printd_mfs2("MINIXfs: m_getblk returning %x for blk %d\n", bh, block);
		return bh;	
	}
	else 
		return NULL;
}

struct buffer_head * minix_bread(inode,block,create)
struct inode * inode;
unsigned short block;
int create;
{
	register struct buffer_head * bh;

	printd_mfs3("mfs: minix_bread(%d, %d, %d)\n", inode, block, create);
	if (!(bh = minix_getblk(inode,block,create))) return NULL;
	printd_mfs2("MINIXfs: Reading block #%d with buffer #%x\n", block, bh);
	return readbuf(bh);
}

/*
 * The minix V1 function to read an inode.
 */
 
static void minix_read_inode(inode)
register struct inode * inode;
{
	struct buffer_head * bh;
	struct minix_inode * raw_inode;
	unsigned short block;
	unsigned int ino;
	static int __c = 0;

	ino = inode->i_ino;
	inode->i_op = NULL;
	inode->i_mode = 0;
	{ /* To isolate register variable */
		register struct super_block * isb = inode->i_sb;
		if (!ino || ino >= isb->u.minix_sb.s_ninodes) {
			printk("Bad inode number on dev %s: %d is out of range\n",
				kdevname(inode->i_dev), ino);
			return;
		}
		block = 2 + isb->u.minix_sb.s_imap_blocks +
			    isb->u.minix_sb.s_zmap_blocks +
			    (ino-1)/MINIX_INODES_PER_BLOCK;
	}
	if (!(bh=bread(inode->i_dev,(unsigned long)block))) {
		printk("Major problem: unable to read inode from dev %s\n", kdevname(inode->i_dev));
		return;
	}
	map_buffer(bh);
	raw_inode = ((struct minix_inode *) bh->b_data) +
		    (ino-1)%MINIX_INODES_PER_BLOCK;
	memcpy(inode, raw_inode, sizeof(struct minix_inode));
	inode->i_ctime = inode->i_atime = inode->i_mtime; 
#ifdef BLOAT_FS
	inode->i_blocks = inode->i_blksize = 0;
#endif
	if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode))
		inode->i_rdev = to_kdev_t(raw_inode->i_zone[0]);
	else for (block = 0; block < 9; block++)
		inode->i_zone[block] = raw_inode->i_zone[block];
	unmap_brelse(bh);
	if (S_ISREG(inode->i_mode))
		inode->i_op = &minix_file_inode_operations;
	else if (S_ISDIR(inode->i_mode))
		inode->i_op = &minix_dir_inode_operations;
	else if (S_ISLNK(inode->i_mode))
		inode->i_op = &minix_symlink_inode_operations;
	else if (S_ISCHR(inode->i_mode))
		inode->i_op = &chrdev_inode_operations;
	else if (S_ISBLK(inode->i_mode))
		inode->i_op = &blkdev_inode_operations;
#ifdef NOT_YET		
	else if (S_ISFIFO(inode->i_mode))
		init_fifo(inode);
#endif		
}

/*
 * The minix V1 function to synchronize an inode.
 */
 
#ifndef CONFIG_FS_RO
static struct buffer_head * minix_update_inode(inode)
register struct inode * inode;
{
	register struct buffer_head * bh;
	struct minix_inode * raw_inode;
	unsigned int ino;
	unsigned short block;

	ino = inode->i_ino;
	if (!ino || ino >= inode->i_sb->u.minix_sb.s_ninodes) {
		printk("Bad inode number on dev %s: %d is out of range\n",
			kdevname(inode->i_dev), ino);
		inode->i_dirt = 0;
		return 0;
	}
	block = 2 + inode->i_sb->u.minix_sb.s_imap_blocks + inode->i_sb->u.minix_sb.s_zmap_blocks +
		(ino-1)/MINIX_INODES_PER_BLOCK;

	if (!(bh=bread(inode->i_dev, (unsigned long)block))) {
		printk("unable to read i-node block\n");
		inode->i_dirt = 0;
		return 0;
	}
	map_buffer(bh);
	raw_inode = ((struct minix_inode *)bh->b_data) +
		(ino-1)%MINIX_INODES_PER_BLOCK;
	memcpy(raw_inode, inode, sizeof(struct minix_inode));
	if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode))
		raw_inode->i_zone[0] = kdev_t_to_nr(inode->i_rdev);
	inode->i_dirt=0;
	mark_buffer_dirty(bh, 1);
	unmap_buffer(bh);
	return bh;

}

void minix_write_inode(inode)
struct inode * inode;
{
	register struct buffer_head *bh;

	bh = minix_update_inode(inode);
	brelse(bh);
}
#endif

#ifdef BLOAT_FS
int minix_sync_inode(inode)
register struct inode * inode;
{
	int err = 0;
	struct buffer_head *bh;

	bh = minix_update_inode(inode);
	if (bh && buffer_dirty(bh))
	{
		ll_rw_blk(WRITE, bh);
		wait_on_buffer(bh);
		if (!buffer_uptodate(bh))
		{
			printk ("IO error syncing minix inode [%s:%08lx]\n",
				kdevname(inode->i_dev), inode->i_ino);
			err = -1;
		}
	}
	else if (!bh)
		err = -1;
	brelse (bh);
	return err;
}
#endif

struct file_system_type minix_fs_type = 
{
#ifdef BLOAT_FS
	minix_read_super, "minix", 1
#else
	minix_read_super, "minix"
#endif
};

int init_minix_fs()
{
        return 1;
        /*register_filesystem(&minix_fs_type);*/
}

