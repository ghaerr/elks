/*
 *  linux/fs/minix/inode.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 */

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/elksfs_fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>
#include <linuxmt/debug.h>

#include <arch/system.h>
#include <arch/segment.h>
#include <arch/bitops.h>

void elksfs_put_inode(inode)
register struct inode *inode;
{
	if (inode->i_nlink)
		return;
	inode->i_size = 0;
#ifndef CONFIG_FS_RO
	elksfs_truncate(inode);
#endif
	elksfs_free_inode(inode);
}

#ifndef CONFIG_FS_RO
static void elksfs_commit_super (sb)
register struct super_block * sb;
/*struct minix_super_block * ms; */
{
	mark_buffer_dirty(sb->u.elksfs_sb.s_sbh, 1);
	sb->s_dirt = 0;
}

void elksfs_write_super (sb)
register struct super_block * sb;
{
	register struct elksfs_super_block * ms;
	if (!(sb->s_flags & MS_RDONLY)) {
		ms = sb->u.elksfs_sb.s_ms;

		if (ms->s_state & ELKSFS_VALID_FS)
			ms->s_state &= ~ELKSFS_VALID_FS;
		elksfs_commit_super (sb);
	}
	sb->s_dirt = 0;
}
#endif

void elksfs_put_super(sb)
register struct super_block *sb;
{
	int i;

	lock_super(sb);
#ifdef CONFIG_FS_RO
	if (!(sb->s_flags & MS_RDONLY)) {
		sb->u.elksfs_sb.s_ms->s_state = sb->u.elksfs_sb.s_mount_state;
		mark_buffer_dirty(sb->u.elksfs_sb.s_sbh, 1);
	}
#endif
	sb->s_dev = 0;
	for(i = 0 ; i < ELKSFS_I_MAP_SLOTS ; i++)
		brelse(sb->u.elksfs_sb.s_imap[i]);
	for(i = 0 ; i < ELKSFS_Z_MAP_SLOTS ; i++)
		brelse(sb->u.elksfs_sb.s_zmap[i]);
	brelse (sb->u.elksfs_sb.s_sbh);
	unlock_super(sb);
	return;
}

static struct super_operations elksfs_sops = { 
	elksfs_read_inode,
#ifdef BLOAT_FS
	NULL,
#endif
#ifdef CONFIG_FS_RO
	NULL,
#else
	elksfs_write_inode,
#endif
	elksfs_put_inode,
	elksfs_put_super,
#ifdef CONFIG_FS_RO
	NULL,
#else
	elksfs_write_super,
#endif
#ifdef BLOAT_FS
	elksfs_statfs,
#endif
	elksfs_remount
};

int elksfs_remount (sb,flags,data)
register struct super_block * sb;
int * flags;
char * data;
{
	register struct elksfs_super_block * ms;

	ms = sb->u.elksfs_sb.s_ms;

#ifdef CONFIG_FS_RO
	if (!(*flags & MS_RDONLY))
		return -EROFS;
#else
	if ((*flags & MS_RDONLY) == (sb->s_flags & MS_RDONLY))
		return 0;
	if (*flags & MS_RDONLY) {
		if (ms->s_state & ELKSFS_VALID_FS ||
		    !(sb->u.elksfs_sb.s_mount_state & ELKSFS_VALID_FS))
			return 0;
		/* Mounting a rw partition read-only. */
		ms->s_state = sb->u.elksfs_sb.s_mount_state;
		mark_buffer_dirty(sb->u.elksfs_sb.s_sbh, 1);
		sb->s_dirt = 1;
		elksfs_commit_super (sb);
	}
	else {
	  	/* Mount a partition which is read-only, read-write. */
		sb->u.elksfs_sb.s_mount_state = ms->s_state;
		ms->s_state &= ~ELKSFS_VALID_FS;
		mark_buffer_dirty(sb->u.elksfs_sb.s_sbh, 1);
		sb->s_dirt = 1;

		if (!(sb->u.elksfs_sb.s_mount_state & ELKSFS_VALID_FS))
			printk ("ELKSFS-fs warning: remounting unchecked fs, running fsck is recommended.\n");
		else if ((sb->u.elksfs_sb.s_mount_state & ELKSFS_ERROR_FS))
			printk ("ELKSFS-fs warning: remounting fs with errors, running fsck is recommended.\n");
	}
#endif
	return 0;
}

struct super_block *elksfs_read_super(s,data,silent)
register struct super_block *s;
char *data;
int silent;
{
	struct buffer_head *bh;
	struct elksfs_super_block *ms;
	int i;
	unsigned long block;
	kdev_t dev = s->s_dev;

	if (32 != sizeof (struct elksfs_inode))
		panic("bad i-node size");
	lock_super(s);
	if (!(bh = bread(dev,1L))) {
		s->s_dev = 0;
		unlock_super(s);
		printk("ELKSFS-fs: unable to read superblock\n");
		return NULL;
	}
	map_buffer(bh);
	ms = (struct elksfs_super_block *) bh->b_data;
	s->u.elksfs_sb.s_ms = ms;
	s->u.elksfs_sb.s_sbh = bh;
	s->u.elksfs_sb.s_mount_state = ms->s_state;
	s->u.elksfs_sb.s_ninodes = ms->s_ninodes;
	s->u.elksfs_sb.s_imap_blocks = ms->s_imap_blocks;
	s->u.elksfs_sb.s_zmap_blocks = ms->s_zmap_blocks;
	s->u.elksfs_sb.s_firstdatazone = ms->s_firstdatazone;
	s->u.elksfs_sb.s_log_zone_size = ms->s_log_zone_size;
	s->u.elksfs_sb.s_max_size = ms->s_max_size;
	s->s_magic = ms->s_magic;
	if (s->s_magic == ELKSFS_SUPER_MAGIC) {
		s->u.elksfs_sb.s_version = ELKSFS_V1;
		s->u.elksfs_sb.s_nzones = ms->s_nzones;
		s->u.elksfs_sb.s_dirsize = 16;
		s->u.elksfs_sb.s_namelen = 14;
	} else if (s->s_magic == MINIX_SUPER_MAGIC) {
		s->u.elksfs_sb.s_version = MINIX_V1;
		s->u.elksfs_sb.s_nzones = ms->s_nzones;
		s->u.elksfs_sb.s_dirsize = 16;
		s->u.elksfs_sb.s_namelen = 14;
	} else {
		s->s_dev = 0;
		unlock_super(s);
		unmap_brelse(bh);
		if (!silent)
			printk("VFS: Can't find a Elksfs V1 filesystem on dev %s.\n", kdevname(dev));
		return NULL;
	}
	for (i=0;i < ELKSFS_I_MAP_SLOTS;i++)
		s->u.elksfs_sb.s_imap[i] = NULL;
	for (i=0;i < ELKSFS_Z_MAP_SLOTS;i++)
		s->u.elksfs_sb.s_zmap[i] = NULL;
	block=2;
	/*
	 *	FIXME:: We cant keep these in memory on an 8086, need to change
	 *	the code to fetch/release each time we get a block.
	 */
	for (i=0 ; i < s->u.elksfs_sb.s_imap_blocks ; i++)
		if ((s->u.elksfs_sb.s_imap[i]=bread(dev,block)) != NULL)
			block++;
		else
			break;
	for (i=0 ; i < s->u.elksfs_sb.s_zmap_blocks ; i++)
		if ((s->u.elksfs_sb.s_zmap[i]=bread(dev,block)) != NULL)
			block++;
		else
			break;
	if (block != 2+s->u.elksfs_sb.s_imap_blocks+s->u.elksfs_sb.s_zmap_blocks) {
		for(i=0;i<ELKSFS_I_MAP_SLOTS;i++)
			brelse(s->u.elksfs_sb.s_imap[i]);
		for(i=0;i<ELKSFS_Z_MAP_SLOTS;i++)
			brelse(s->u.elksfs_sb.s_zmap[i]);
		s->s_dev = 0;
		unlock_super(s);
		unmap_brelse(bh);
		printk("elksfs: bad superblock or bitmaps\n");
		return NULL;
	}
	set_bit(0,s->u.elksfs_sb.s_imap[0]->b_data);
	set_bit(0,s->u.elksfs_sb.s_zmap[0]->b_data);
	unlock_super(s);
	/* set up enough so that it can read an inode */
	s->s_dev = dev;
	s->s_op = &elksfs_sops;
	s->s_mounted = iget(s,(ino_t) ELKSFS_ROOT_INO);
	if (!s->s_mounted) {
		s->s_dev = 0;
		unmap_brelse(bh);
		printk("elksfs: get root inode failed\n");
		return NULL;
	}
	if (!(s->s_flags & MS_RDONLY)) {
		ms->s_state &= ~ELKSFS_VALID_FS;
		mark_buffer_dirty(bh, 1);
		s->s_dirt = 1;
	}
	if (!(s->u.elksfs_sb.s_mount_state & ELKSFS_VALID_FS))
		printk ("ELKSFS-fs: mounting unchecked file system, running fsck is recommended.\n");
 	else if (s->u.elksfs_sb.s_mount_state & ELKSFS_ERROR_FS)
		printk ("ELKSFS-fs: mounting file system with errors, running fsck is recommended.\n");
	unmap_buffer(bh);
	return s;
}

#ifdef BLOAT_FS
void elksfs_statfs(sb,buf,bufsiz)
register struct super_block *sb;
struct statfs *buf;
int bufsiz;
{
	struct statfs tmp;

	tmp.f_type = sb->s_magic;
	tmp.f_bsize = BLOCK_SIZE;
	tmp.f_blocks = (sb->u.elksfs_sb.s_nzones - sb->u.elksfs_sb.s_firstdatazone) << sb->u.elksfs_sb.s_log_zone_size;
	tmp.f_bfree = elksfs_count_free_blocks(sb);
	tmp.f_bavail = tmp.f_bfree;
	tmp.f_files = sb->u.elksfs_sb.s_ninodes;
	tmp.f_ffree = elksfs_count_free_inodes(sb);
	tmp.f_namelen = sb->u.elksfs_sb.s_namelen;
	memcpy(buf, &tmp, bufsiz);
}
#endif

/* Adapted from Linux 0.12's inode.c.  _bmap() is a long function, I know */

unsigned long _elksfs_bmap(inode, block, create)
register struct inode * inode;
unsigned long block;
int create;
{
	struct buffer_head * bh;
	register unsigned short *i_zone = inode->i_zone;
	unsigned long i;

	if (block > (7+512+512*512))	
		panic("_elksfs_bmap: block (%ld) >big", block);

	if (block < 7) {
#ifndef CONFIG_FS_RO
		if (create && !i_zone[block]) {
			if (i_zone[block]=elksfs_new_block(inode->i_sb)) {
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
				if (i_zone[7]=elksfs_new_block(inode->i_sb)) {
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
			if (i=elksfs_new_block(inode->i_sb)) {
				((unsigned short *) (bh->b_data))[block] = i;
				bh->b_dirty = 1;
			}
		}
#endif
		unmap_brelse(bh);
		printd_mfs1("MFSbmap: Returning #%ld\n", i);
		return i;
	}
	printk("ELKSFS-fs: bmap cannot handle > 519K files yet!\n");
	return 0;
}

struct buffer_head * elksfs_getblk(inode,block,create)
register struct inode * inode;
unsigned long block;
int create;
{
	struct buffer_head * bh;
	unsigned long blknum;

	blknum = _elksfs_bmap(inode, block, create);
	printd_mfs2("ELKSFSfs: file block #%ld -> disk block #%ld\n", block, blknum);
	if (blknum != 0) {
		bh = getblk(inode->i_dev, blknum); 
		printd_mfs2("ELKSFSfs: m_getblk returning %x for blk %d\n", bh, block);
		return bh;	
	}
	else 
		return NULL;
}

struct buffer_head * elksfs_bread(inode,block,create)
struct inode * inode;
int block;
int create;
{
	register struct buffer_head * bh;
	register struct buffer_head * bha;

	printd_mfs3("mfs: elksfs_bread(%d, %d, %d)\n", inode, block, create);
	if (!(bh = elksfs_getblk(inode,(long)block,create))) return NULL;
	printd_mfs2("ELKSFSfs: Reading block #%d with buffer #%x\n", block, bh);
	return readbuf(bh);
}

/*
 * The minix V1 function to read an inode.
 */
 
static void elksfs_read_inode(inode)
register struct inode * inode;
{
	struct buffer_head * bh;
	struct elksfs_inode * raw_inode;
	unsigned long block;
	unsigned int ino;
	static int __c = 0;

	ino = inode->i_ino;
	inode->i_op = NULL;
	inode->i_mode = 0;
	if (!ino || ino >= inode->i_sb->u.elksfs_sb.s_ninodes) {
		printk("Bad inode number on dev %s: %d is out of range\n",
			kdevname(inode->i_dev), ino);
		return;
	}
	block = 2 + inode->i_sb->u.elksfs_sb.s_imap_blocks +
		    inode->i_sb->u.elksfs_sb.s_zmap_blocks +
		    (ino-1)/ELKSFS_INODES_PER_BLOCK;
	if (!(bh=bread(inode->i_dev,block))) {
		printk("Major problem: unable to read inode from dev %s\n", kdevname(inode->i_dev));
		return;
	}
	map_buffer(bh);
	raw_inode = ((struct elksfs_inode *) bh->b_data) +
		    (ino-1)%ELKSFS_INODES_PER_BLOCK;
	memcpy(inode, raw_inode, sizeof(struct elksfs_inode));
	inode->i_ctime = inode->i_atime = inode->i_mtime; 
#ifdef BLOAT_FS
	inode->i_blocks = inode->i_blksize = 0;
#else
	inode->i_blksize = 0;
#endif
	if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode))
		inode->i_rdev = to_kdev_t(raw_inode->i_zone[0]);
	else for (block = 0; block < 9; block++)
		inode->i_zone[block] = raw_inode->i_zone[block];
	unmap_brelse(bh);
	if (S_ISREG(inode->i_mode))
		inode->i_op = &elksfs_file_inode_operations;
	else if (S_ISDIR(inode->i_mode))
		inode->i_op = &elksfs_dir_inode_operations;
	else if (S_ISLNK(inode->i_mode))
		inode->i_op = &elksfs_symlink_inode_operations;
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
static struct buffer_head * elksfs_update_inode(inode)
register struct inode * inode;
{
	register struct buffer_head * bh;
	struct elksfs_inode * raw_inode;
	unsigned int ino;
	unsigned long block;

	ino = inode->i_ino;
	if (!ino || ino >= inode->i_sb->u.elksfs_sb.s_ninodes) {
		printk("Bad inode number on dev %s: %d is out of range\n",
			kdevname(inode->i_dev), ino);
		inode->i_dirt = 0;
		return 0;
	}
	block = 2 + inode->i_sb->u.elksfs_sb.s_imap_blocks + inode->i_sb->u.elksfs_sb.s_zmap_blocks +
		(ino-1)/ELKSFS_INODES_PER_BLOCK;
	if (!(bh=bread(inode->i_dev, block))) {
		printk("unable to read i-node block\n");
		inode->i_dirt = 0;
		return 0;
	}
	map_buffer(bh);
	raw_inode = ((struct elksfs_inode *)bh->b_data) +
		(ino-1)%ELKSFS_INODES_PER_BLOCK;
	memcpy(raw_inode, inode, sizeof(struct elksfs_inode));
	if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode))
		raw_inode->i_zone[0] = kdev_t_to_nr(inode->i_rdev);
	inode->i_dirt=0;
	mark_buffer_dirty(bh, 1);
	unmap_buffer(bh);
	return bh;
}

void elksfs_write_inode(inode)
struct inode * inode;
{
	register struct buffer_head *bh;

	bh = elksfs_update_inode(inode);
	brelse(bh);
}
#endif

#ifdef BLOAT_FS
int elksfs_sync_inode(inode)
register struct inode * inode;
{
	int err = 0;
	struct buffer_head *bh;

	bh = elksfs_update_inode(inode);
	if (bh && buffer_dirty(bh))
	{
		ll_rw_blk(WRITE, bh);
		wait_on_buffer(bh);
		if (!buffer_uptodate(bh))
		{
			printk ("IO error syncing elksfs inode [%s:%08lx]\n",
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

struct file_system_type elksfs_fs_type = 
{
#ifdef BLOAT_FS
	elksfs_read_super, "elksfs", 1
#else
	elksfs_read_super, "elksfs"
#endif
};

int init_elksfs_fs()
{
        return 1;
        /*register_filesystem(&elksfs_fs_type);*/
}

