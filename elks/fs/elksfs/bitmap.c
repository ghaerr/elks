/*
 *  linux/fs/minix/bitmap.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* bitmap.c contains the code that handles the inode and block bitmaps */

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/elksfs_fs.h>
#include <linuxmt/stat.h>
#include <linuxmt/kernel.h>
#include <linuxmt/string.h>
#include <linuxmt/fs.h>

#include <arch/bitops.h>

static int nibblemap[] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };

#ifdef BLOAT_FS
static unsigned long count_used(map,numblocks,numbits)
struct buffer_head *map[];
unsigned int numblocks;
unsigned int numbits;
{
	unsigned int i, j, end, sum = 0;
	register struct buffer_head *bh;
  
	for (i=0; (i<numblocks) && numbits; i++) 
	{
		if (!(bh=map[i])) 
			return(0);
		map_buffer(bh);
		if (numbits >= (8*BLOCK_SIZE)) { 
			end = BLOCK_SIZE;
			numbits -= 8*BLOCK_SIZE;
		} else {
			int tmp;
			end = numbits >> 3;
			tmp = bh->b_data[end] & ((1<<(numbits&7))-1);
			sum += nibblemap[tmp&0xf] + nibblemap[(tmp>>4)&0xf];
			numbits = 0;
		}  
		for (j=0; j<end; j++)
			sum += nibblemap[bh->b_data[j] & 0xf] 
				+ nibblemap[(bh->b_data[j]>>4)&0xf];
		unmap_buffer(bh);
	}
	return(sum);
}
#endif

#ifndef CONFIG_FS_RO
void elksfs_free_block(sb,block)
register struct super_block * sb;
unsigned long block;
{
	register struct buffer_head * bh;
	unsigned int bit,zone;

	if (!sb) {
		printk("trying to free block on nonexistent device\n");
		return;
	}
	if (block < sb->u.elksfs_sb.s_firstdatazone ||
	    block >= sb->u.elksfs_sb.s_nzones) {
		printk("trying to free block %x not in datazone\n", block);
		return;
	}
	bh = get_hash_table(sb->s_dev,block);
	if (bh)
		bh->b_dirty = 0;
	brelse(bh);
	zone = block - sb->u.elksfs_sb.s_firstdatazone + 1;
	bit = zone & 8191;
	zone >>= 13;
	bh = sb->u.elksfs_sb.s_zmap[zone];
	if (!bh) {
		printk("elksfs_free_block: nonexistent bitmap buffer\n");
		return;
	}
	map_buffer(bh);
/*	if (!clear_bit(bit,bh->b_data))
		printk("free_block (%s:%ld): bit already cleared\n",
		       kdevname(sb->s_dev), block);
*/	
	clear_bit(bit, bh->b_data);
	unmap_buffer(bh);
	mark_buffer_dirty(bh, 1);
	return;
}

unsigned int elksfs_new_block(sb)
register struct super_block * sb;
{
	register struct buffer_head * bh;
	unsigned int i, j;

	if (!sb) {
		printk("trying to get new block from nonexistent device\n");
		return 0;
	}
repeat:
	j = 8192;
	for (i=0 ; i<8 ; i++)
		if ((bh=sb->u.elksfs_sb.s_zmap[i]) != NULL) {
			map_buffer(bh);
			if ((j=find_first_zero_bit(bh->b_data, 8192)) < 8192)
				break;
			unmap_buffer(bh);
		}
	if (i>=8 || !bh || j>=8192)
		return 0;
	if (set_bit(j,bh->b_data)) {
		panic("new_block: bit already set %d %d\n", j, bh->b_data);
		unmap_buffer(bh);
		goto repeat;
	}
	if (j == find_first_zero_bit(bh->b_data, 8192))
		panic("still zero bit!%d\n", j);
	unmap_buffer(bh);
	mark_buffer_dirty(bh, 1);
	j += i*8192 + sb->u.minix_sb.s_firstdatazone-1;
	if (j < sb->u.elksfs_sb.s_firstdatazone ||
	    j >= sb->u.elksfs_sb.s_nzones)
		return 0;
	/* WARNING: j is not converted, so we have to do it */
	if (!(bh = getblk(sb->s_dev, (long)j))) {
		printk("new_block: cannot get block");
		return 0;
	}
	map_buffer(bh);
	memset(bh->b_data, 0, BLOCK_SIZE);
	mark_buffer_uptodate(bh, 1);
	mark_buffer_dirty(bh, 1);
	unmap_brelse(bh);
	return j;
}
#endif /* CONFIG_FS_RO */

#ifdef BLOAT_FS
unsigned long elksfs_count_free_blocks(sb)
register struct super_block *sb;
{
	return (sb->u.elksfs_sb.s_nzones - count_used(sb->u.elksfs_sb.s_zmap,sb->u.elksfs_sb.s_zmap_blocks,sb->u.elksfs_sb.s_nzones))
		 << sb->u.elksfs_sb.s_log_zone_size;
}
#endif

static char *fi_name = "free_inode";
void elksfs_free_inode(inode)
register struct inode * inode;
{
#ifdef CONFIG_FS_RO
	clear_inode(inode);
#else
	register struct buffer_head * bh;
	unsigned long ino;

	if (!inode)
		return;
	if (!inode->i_dev) {
		printk("%s: inode has no device\n",fi_name);
		return;
	}
	if (inode->i_count != 1) {
		printk("%s: inode has count=%d\n",fi_name,inode->i_count);
		return;
	}
	if (inode->i_nlink) {
		printk("%s: inode has nlink=%d\n",fi_name,inode->i_nlink);
		return;
	}
	if (!inode->i_sb) {
		printk("%s: inode on nonexistent device\n",fi_name);
		return;
	}
	if (inode->i_ino < 1 || inode->i_ino >= inode->i_sb->u.elksfs_sb.s_ninodes) {
		printk("%s: inode 0 or nonexistent inode\n",fi_name);
		return;
	}
	ino = inode->i_ino;
	if (!(bh=inode->i_sb->u.elksfs_sb.s_imap[ino >> 13])) {
		printk("%s: nonexistent imap in superblock\n",fi_name);
		return;
	}
	map_buffer(bh);
	clear_inode(inode);
	if (!clear_bit(ino & 8191, bh->b_data)) {
/*		printk("%s: bit %ld already cleared.\n",ino); */
	}
	mark_buffer_dirty(bh, 1);
	unmap_buffer(bh);
#endif
}

#ifndef CONFIG_FS_RO
struct inode * elksfs_new_inode(dir)
struct inode * dir;
{
	struct super_block * sb;
	register struct inode * inode;
	register struct buffer_head * bh;
	int i,j;

	if (!dir || !(inode = get_empty_inode()))
		return NULL;
	sb = dir->i_sb;
	inode->i_sb = sb;
/*	inode->i_flags = inode->i_sb->s_flags; Changed to line below */
	inode->i_flags = sb->s_flags;
	j = 8192;
	for (i=0 ; i<8 ; i++)
/*		if ((bh = inode->i_sb->u.elksfs_sb.s_imap[i]) != NULL) { */
		if ((bh = sb->u.elksfs_sb.s_imap[i]) != NULL) {
			map_buffer(bh);
			if ((j=find_first_zero_bit(bh->b_data, 8192)) < 8192)
				break;
			unmap_buffer(bh);
		}
	if (!bh || j >= 8192) {
		printk("No new inodes found!\n");
		goto iputfail;	
	}
	if (set_bit(j,bh->b_data)) {	/* shouldn't happen */
		printk("new_inode: bit already set");
		goto iputfail;
	}
	mark_buffer_dirty(bh, 1);
	j += i*8192;
/*	if (!j || j >= inode->i_sb->u.elksfs_sb.s_ninodes) { */
	if (!j || j >= sb->u.elksfs_sb.s_ninodes) {
		goto iputfail;	
	}
	unmap_buffer(bh);
	inode->i_count = 1;
	inode->i_nlink = 1;
	inode->i_dev = sb->s_dev;
	inode->i_uid = current->euid;
	inode->i_gid = (dir->i_mode & S_ISGID) ? dir->i_gid : current->egid;
	inode->i_dirt = 1;
	inode->i_ino = j;
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
	inode->i_op = NULL;
#ifdef BLOAT_FS
	inode->i_blocks = inode->i_blksize = 0;
#else
	inode->i_blksize = 0;
#endif
	insert_inode_hash(inode);
	return inode;
	/* Oh no! It's 'Return of Goto' in a double feature with 'Mozilla vs.
	   Internet Exploder :) */
	iputfail:
	printk("new_inode: Ack ack!  This sucked...\n");
	unmap_buffer(bh); iput(inode); return NULL;
}
#endif /* CONFIG_FS_RO */

#ifdef BLOAT_FS
unsigned long elksfs_count_free_inodes(sb)
register struct super_block *sb;
{
	return sb->u.elksfs_sb.s_ninodes - count_used(sb->u.elksfs_sb.s_imap,sb->u.elksfs_sb.s_imap_blocks,sb->u.elksfs_sb.s_ninodes);
}
#endif
