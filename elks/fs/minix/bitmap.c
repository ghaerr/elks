/*
 *  linux/fs/minix/bitmap.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 * Aug 2023 Greg Haerr - Don't use L1 cache/memset on new filesystem blocks.
 * Aug 2023 Greg Haerr - Don't dedicate buffers for Z/I maps.
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

static char nibblemap[] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };

struct buffer_head *get_map_block(kdev_t dev, block_t block)
{
    struct buffer_head *bh;

    bh = get_hash_table(dev, block);
    if (bh)
        bh = readbuf(bh);
    else
        bh = bread(dev, block);
    if (!EBH(bh)->b_uptodate) {
        debug_blk("get_map_block: can't read block %D/%u\n", dev, block);
        brelse(bh);
        return NULL;
    }
    return bh;
}

static unsigned short count_used(kdev_t dev, block_t map[], unsigned int numblocks,
        unsigned int numbits)
{
    unsigned int i, j, end, sum = 0;
    register struct buffer_head *bh;

    for (i = 0; (i < numblocks) && numbits; i++) {
	if (!(bh = get_map_block(dev, map[i]))) return 0;
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
	    sum += nibblemap[bh->b_data[j] & 0xf] + nibblemap[(bh->b_data[j] >> 4) & 0xf];
	unmap_brelse(bh);
    }
    return sum;
}

unsigned short minix_count_free_blocks(register struct super_block *sb)
{
    return (sb->u.minix_sb.s_nzones -
	count_used(sb->s_dev, sb->u.minix_sb.s_zmap, sb->u.minix_sb.s_zmap_blocks,
	    sb->u.minix_sb.s_nzones)) << sb->u.minix_sb.s_log_zone_size;
}

unsigned short minix_count_free_inodes(register struct super_block *sb)
{
    return (sb->u.minix_sb.s_ninodes -
	count_used(sb->s_dev, sb->u.minix_sb.s_imap, sb->u.minix_sb.s_imap_blocks,
	    sb->u.minix_sb.s_ninodes));
}

void minix_free_block(register struct super_block *sb, unsigned short block)
{
    register struct buffer_head *bh;
    const char *s = NULL;
    unsigned int zone;

    if (!sb) return;
    if (block < sb->u.minix_sb.s_firstdatazone || block >= sb->u.minix_sb.s_nzones)
	s = "not in datazone";
    else {
	bh = get_hash_table(sb->s_dev, (block_t) block);
	if (bh) mark_buffer_clean(bh);
	brelse(bh);
	zone = block - sb->u.minix_sb.s_firstdatazone + 1;
	bh = get_map_block(sb->s_dev, sb->u.minix_sb.s_zmap[zone >> 13]);
	if (!bh) s = "null zmap";
	else {
	    map_buffer(bh);
	    if (!clear_bit(zone & 8191, bh->b_data))
		s = "already cleared";
	    mark_buffer_dirty(bh);
	    unmap_brelse(bh);
	}
    }
    if (s)
	printk("free_block: block %u %s\n", block, s);
}

block_t minix_new_block(struct super_block *sb)
{
    struct buffer_head *bh = NULL;
    block_t i, j;
    if (!sb) return 0;

repeat:
    j = 8192;
    for (i = 0; i < sb->u.minix_sb.s_zmap_blocks; i++) {
        if (i > 0)
            unmap_brelse(bh);
        if ((bh = get_map_block(sb->s_dev, sb->u.minix_sb.s_zmap[i])) != NULL) {
            map_buffer(bh);
            if ((j = find_first_zero_bit((void *)bh->b_data, 8192)) < 8192)
                break;
        }
    }
    if (i >= sb->u.minix_sb.s_zmap_blocks || !bh || j >= 8192) {
        if (bh) unmap_brelse(bh);
        return 0;
    }
    if (set_bit(j, bh->b_data)) {
        printk("new_block: bit already set %d\n", j);
        unmap_brelse(bh);
        goto repeat;
    }
    mark_buffer_dirty(bh);
    unmap_brelse(bh);
    j += i*8192 + sb->u.minix_sb.s_firstdatazone - 1;
    if (j < sb->u.minix_sb.s_firstdatazone || j >= sb->u.minix_sb.s_nzones) {
        return 0;
    }
    if (!(bh = getblk(sb->s_dev, j))) {
        printk("new_block: bad block %u\n", j);
        return 0;
    }
    debug_blk("minix_new_block: block %ld uptodate %d\n",
        EBH(bh)->b_blocknr, EBH(bh)->b_uptodate);
    zero_buffer(bh, 0, BLOCK_SIZE);
    mark_buffer_uptodate(bh, 1);
    mark_buffer_dirty(bh);
    brelse(bh);
    return j;
}

void minix_free_inode(register struct inode *inode)
{
    struct buffer_head *bh;
    const char *s = NULL;
    int n = 0;

    if (!inode->i_dev) s = "no dev\n";
    else if (inode->i_count != 1) {
	n = inode->i_count;
	s = "count %d\n";
    }
    else if (inode->i_nlink) {
	n = inode->i_nlink;
	s = "nlink %d\n";
    }
    else if (!inode->i_sb) s = "no sb\n";
    else if ((int)inode->i_ino == 0) s = "inode 0\n";
    else if ((unsigned int)inode->i_ino > inode->i_sb->u.minix_sb.s_ninodes)
	s = "nonexistent inode\n";
    else if (!(bh = get_map_block(inode->i_sb->s_dev, inode->i_sb->u.minix_sb.s_imap[(unsigned int)inode->i_ino >> 13])))
	s = "nonexistent imap\n";
    if (s) {
	printk("free_inode: ");
	printk(s, n);
    } else {
	map_buffer(bh);
	if (!clear_bit((int)inode->i_ino & 8191, bh->b_data))
	    printk("free_inode: already cleared %d\n", (int)inode->i_ino & 8191);
	clear_inode(inode);
	mark_buffer_dirty(bh);
	unmap_brelse(bh);
    }
}

struct inode *minix_new_inode(struct inode *dir, mode_t mode)
{
    struct inode *inode;
    struct super_block *sb;
    struct buffer_head *bh = NULL;
    block_t i, j;

    if (!dir || !(inode = new_inode(dir, mode)))
        return NULL;
    minix_set_ops(inode);
    sb = inode->i_sb;

    j = 8192;
    for (i = 0; i < sb->u.minix_sb.s_imap_blocks; i++) {
        if (i > 0)
            unmap_brelse(bh);
        if ((bh = get_map_block(sb->s_dev, sb->u.minix_sb.s_imap[i])) != NULL) {
            map_buffer(bh);
            if ((j = find_first_zero_bit((void *)bh->b_data, 8192)) < 8192)
                break;
        }
    }
    if (i >= sb->u.minix_sb.s_imap_blocks || !bh || j >= 8192)
        goto errout;

    j += i*8192;
    if (!j || j >= sb->u.minix_sb.s_ninodes)
        goto errout;

    if (set_bit(j & 8191, bh->b_data)) {    /* shouldn't happen */
        printk("new_inode: bit already set %d\n", j);
        goto errout2;
    }
    mark_buffer_dirty(bh);
    unmap_brelse(bh);
    inode->i_dirt = 1;
    inode->i_ino = j;
    return inode;

errout:
    printk("new_inode: Out of inodes\n");
errout2:
    if (bh)
        unmap_brelse(bh);
    iput(inode);
    return NULL;
}
