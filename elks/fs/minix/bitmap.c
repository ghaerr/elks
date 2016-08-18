/*
 *  linux/fs/minix/bitmap.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* bitmap.c contains the code that handles the inode and block bitmaps */

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/minix_fs.h>
#include <linuxmt/stat.h>
#include <linuxmt/kernel.h>
#include <linuxmt/string.h>
#include <linuxmt/fs.h>
#include <linuxmt/debug.h>

#include <arch/bitops.h>

#ifdef BLOAT_FS

static char nibblemap[] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };

static unsigned short count_used(struct buffer_head *map[],
				 unsigned int numblocks, unsigned int numbits)
{
    unsigned int i, j, end, sum = 0;
    register struct buffer_head *bh;

    for (i = 0; (i < numblocks) && numbits; i++) {
	if (!(bh = map[i]))
	    return 0;
	map_buffer(bh);
	if (numbits >= (8 * BLOCK_SIZE)) {
	    end = BLOCK_SIZE;
	    numbits -= 8 * BLOCK_SIZE;
	} else {
	    int tmp;
	    end = numbits >> 3;
	    tmp = bh->b_data[end] & ((1 << (numbits & 7)) - 1);
	    sum += nibblemap[tmp & 0xf] + nibblemap[(tmp >> 4) & 0xf];
	    numbits = 0;
	}
	for (j = 0; j < end; j++)
	    sum += nibblemap[bh->b_data[j] & 0xf]
		+ nibblemap[(bh->b_data[j] >> 4) & 0xf];
	unmap_buffer(bh);
    }
    return sum;
}

unsigned short minix_count_free_blocks(register struct super_block *sb)
{
    return (sb->u.minix_sb.s_nzones -
	    count_used(sb->u.minix_sb.s_zmap, sb->u.minix_sb.s_zmap_blocks,
		       sb->u.minix_sb.s_nzones)) << sb->u.
	minix_sb.s_log_zone_size;
}


unsigned short minix_count_free_inodes(register struct super_block *sb)
{
    return sb->u.minix_sb.s_ninodes - count_used(sb->u.minix_sb.s_imap,
						 sb->u.minix_sb.s_imap_blocks,
						 sb->u.minix_sb.s_ninodes);
}

#endif

void minix_free_block(register struct super_block *sb, unsigned short block)
{
    register struct buffer_head *bh;
    unsigned int zone;

    if (!sb)
	printk("mfb: bad dev\n");
    else if (block < sb->u.minix_sb.s_firstdatazone ||
	    block >= sb->u.minix_sb.s_nzones)
	printk("trying to free block %lx not in datazone\n", block);
    else {
	bh = get_hash_table(sb->s_dev, (block_t) block);
	if (bh)
	    bh->b_dirty = 0;
	brelse(bh);
	zone = block - sb->u.minix_sb.s_firstdatazone + 1;
	bh = sb->u.minix_sb.s_zmap[zone >> 13];
	if (!bh)
	    printk("mfb: bad bitbuf\n");
	else {
	    map_buffer(bh);
	    if (!clear_bit(zone & 8191, bh->b_data))
		printk("mfb (%s:%ld): already cleared\n", kdevname(sb->s_dev), block);
	    unmap_buffer(bh);
	    mark_buffer_dirty(bh, 1);
	}
    }
}

block_t minix_new_block(register struct super_block *sb)
{
    register struct buffer_head *bh;
    block_t i, j;

    if (!sb) {
	printk("mnb: no sb\n");
	return 0;
    }

  repeat:
    j = 8192;
    for (i = 0; i < 8; i++)
	if ((bh = sb->u.minix_sb.s_zmap[i]) != NULL) {
	    map_buffer(bh);
	    j = (block_t) find_first_zero_bit((void *)(bh->b_data), 8192);
	    if (j < 8192)
		break;
	    unmap_buffer(bh);
	}
    if (i >= 8)
	return 0;
    if (set_bit(j, bh->b_data)) {
	panic("mnb: already set %d %d\n", j, bh->b_data);
	unmap_buffer(bh);
	goto repeat;
    }
    if (j == (block_t) find_first_zero_bit((void *)(bh->b_data), 8192))
	panic("still zero bit!%d\n", j);
    unmap_buffer(bh);
    mark_buffer_dirty(bh, 1);
    j += i * 8192 + sb->u.minix_sb.s_firstdatazone - 1;
    if (j < sb->u.minix_sb.s_firstdatazone || j >= sb->u.minix_sb.s_nzones)
	return 0;
    /* WARNING: j is not converted, so we have to do it */
    bh = getblk(sb->s_dev, (block_t) j);
    map_buffer(bh);
    memset(bh->b_data, 0, BLOCK_SIZE);
    mark_buffer_uptodate(bh, 1);
    mark_buffer_dirty(bh, 1);
    unmap_brelse(bh);
    return j;
}

void minix_free_inode(register struct inode *inode)
{
    struct buffer_head *bh;
    register char *s;
    int n;

    s = 0;
    n = 0;
    if (!inode->i_dev)
	s = "no device\n";
    else if (inode->i_count != 1) {
	n = inode->i_count;
	s = "count=%d\n";
    }
    else if (inode->i_nlink) {
	n = inode->i_nlink;
	s = "nlink=%d\n";
    }
    else if (!inode->i_sb)
	s = "no sb\n";
    else if (inode->i_ino < 1)
	s = "inode 0\n";
    else if (inode->i_ino > inode->i_sb->u.minix_sb.s_ninodes)
	s = "nonexistent inode\n";
    else if (!(bh = inode->i_sb->u.minix_sb.s_imap[inode->i_ino >> 13]))
	s = "nonexistent imap\n";
    else {
	map_buffer(bh);
	clear_inode(inode);
	if (!clear_bit((unsigned int) (inode->i_ino & 8191), bh->b_data)) {
	    debug1("%s: bit %ld already cleared.\n",ino);
	}
	mark_buffer_dirty(bh, 1);
	unmap_buffer(bh);
    }
    if (s) {
	printk("free_inode: ");
	printk(s, n);
    }
}

struct inode *minix_new_inode(struct inode *dir, __u16 mode)
{
    register struct inode *inode;
    register struct buffer_head *bh;
    /* Adding an sb here does not make the code smaller */
    block_t i, j;

    if (!dir || !(inode = new_inode(dir, mode)))
	return NULL;

    minix_set_ops(inode);
    j = 8192;
    for (i = 0; i < 8; i++)
	if ((bh = inode->i_sb->u.minix_sb.s_imap[i]) != NULL) {
	    map_buffer(bh);
	    j = (block_t) find_first_zero_bit((void *)(bh->b_data), 8192);
	    if (j < 8192)
		break;
	    unmap_buffer(bh);
	}
    if (!bh || j >= 8192) {
	printk("No new inodes found!\n");
	goto iputfail1;
    }
    if (set_bit(j, bh->b_data)) {	/* shouldn't happen */
	printk("mni: already set\n");
	goto iputfail;
    }
    mark_buffer_dirty(bh, 1);
    j += i * 8192;
    if (!j || j > inode->i_sb->u.minix_sb.s_ninodes) {
	goto iputfail;
    }
    unmap_buffer(bh);
    inode->i_ino = j;
    inode->i_dirt = 1;

#ifdef BLOAT_FS
    inode->i_blocks = inode->i_blksize = 0;
#endif

    return inode;

    /*  Oh no! We have 'Return of Goto' in a double feature with
     *  'Mozilla v Internet Exploder' :) */

    printk("new_inode: iput fail\n");
  iputfail:
    unmap_buffer(bh);
  iputfail1:
    iput(inode);
    return NULL;
}
