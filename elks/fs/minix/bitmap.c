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

static char nibblemap[] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };

static unsigned short count_used(struct buffer_head *map[],
				 unsigned int numblocks, unsigned int numbits)
{
    unsigned int i, j, end, sum = 0;
    register struct buffer_head *bh;

    for (i = 0; (i < numblocks) && numbits; i++) {
	if (!(bh = map[i])) return 0;
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
	unmap_buffer(bh);
    }
    return sum;
}

unsigned short minix_count_free_blocks(register struct super_block *sb)
{
    return (sb->u.minix_sb.s_nzones -
	count_used(sb->u.minix_sb.s_zmap, sb->u.minix_sb.s_zmap_blocks,
	    sb->u.minix_sb.s_nzones)) << sb->u.minix_sb.s_log_zone_size;
}

unsigned short minix_count_free_inodes(register struct super_block *sb)
{
    return (sb->u.minix_sb.s_ninodes -
	count_used(sb->u.minix_sb.s_imap, sb->u.minix_sb.s_imap_blocks,
	    sb->u.minix_sb.s_ninodes));
}

void minix_free_block(register struct super_block *sb, unsigned short block)
{
    register struct buffer_head *bh;
    char *s;
    unsigned int zone;

    s = NULL;
    if (!sb)
	s = "bad dev";
    else if (block < sb->u.minix_sb.s_firstdatazone ||
	    block >= sb->u.minix_sb.s_nzones)
	s = "trying to free block not in datazone";
    else {
	bh = get_hash_table(sb->s_dev, (block_t) block);
	if (bh) mark_buffer_clean(bh);
	brelse(bh);
	zone = block - sb->u.minix_sb.s_firstdatazone + 1;
	bh = sb->u.minix_sb.s_zmap[zone >> 13];
	if (!bh) s = "bad bitbuf";
	else {
	    map_buffer(bh);
	    if (!clear_bit(zone & 8191, bh->b_data))
		s = "already cleared";
	    mark_buffer_dirty(bh);
	    unmap_buffer(bh);
	}
    }
    if (s != NULL)
	printk("mfb (%s:%u): %s\n", kdevname(sb->s_dev), block, s);
}

block_t minix_new_block(register struct super_block *sb)
{
    register struct buffer_head *bh;
    block_t i, j, k;

    if (!sb) return 0;

    i = 0;
    do {
	bh = sb->u.minix_sb.s_zmap[i];
	map_buffer(bh);
	j = (block_t) find_first_zero_bit((void *)(bh->b_data), 8192);
	k = j + i * 8192 + sb->u.minix_sb.s_firstdatazone - 1;
	if (k < sb->u.minix_sb.s_nzones) {
	    if (set_bit(j, bh->b_data)) {
			printk("mnb: already set %d %d\n", j, bh->b_data);
			return 0;
	    }
	    if (j == (block_t)find_first_zero_bit((void *)(bh->b_data), 8192)) {
			printk("mnb: still zero bit!%d\n", j);
			return 0;
		}
	    mark_buffer_dirty(bh);
	    unmap_buffer(bh);
	    if (!k)
		break;
	    bh = getblk(sb->s_dev, k);
	    map_buffer(bh);
	    memset(bh->b_data, 0, BLOCK_SIZE);
	    mark_buffer_uptodate(bh, 1);
	    mark_buffer_dirty(bh);
	    unmap_brelse(bh);
	    return k;
	}
	unmap_buffer(bh);
    } while (++i < sb->u.minix_sb.s_zmap_blocks);
    return 0;
}

void minix_free_inode(register struct inode *inode)
{
    struct buffer_head *bh;
    register char *s;
    int n;

    s = 0;
    n = 0;
    if (!inode->i_dev) s = "no device\n";
    else if (inode->i_count != 1) {
	n = inode->i_count;
	s = "count=%d\n";
    }
    else if (inode->i_nlink) {
	n = inode->i_nlink;
	s = "nlink=%d\n";
    }
    else if (!inode->i_sb) s = "no sb\n";
    else if ((unsigned int)inode->i_ino < 1) s = "inode 0\n";
    else if ((unsigned int)inode->i_ino > inode->i_sb->u.minix_sb.s_ninodes)
	s = "nonexistent inode\n";
    else if (!(bh = inode->i_sb->u.minix_sb.s_imap[(unsigned int)inode->i_ino >> 13]))
	s = "nonexistent imap\n";
    if (s) {
	printk("free_inode: ");
	printk(s, n);
    }
    else {
	map_buffer(bh);
	if (!clear_bit((unsigned int) ((unsigned int)inode->i_ino & 8191), bh->b_data)) {
	    debug("minix_free_inode: bit %ld already cleared.\n",((unsigned int)inode->i_ino & 8191));
	}
	clear_inode(inode);
	mark_buffer_dirty(bh);
	unmap_buffer(bh);
    }
}

struct inode *minix_new_inode(struct inode *dir, __u16 mode)
{
    register struct inode *inode;
    register struct buffer_head *bh;
    /* Adding an sb here does not make the code smaller */
    block_t i, j, k;
    char *s;

    if (!dir || !(inode = new_inode(dir, mode))) return NULL;
    minix_set_ops(inode);

    s = "No new inodes found!\n";
    i = 0;
    do {
	bh = inode->i_sb->u.minix_sb.s_imap[i];
	map_buffer(bh);
	j = (block_t) find_first_zero_bit((void *)(bh->b_data), 8192);
	k = j + i * 8192;
	if (k < inode->i_sb->u.minix_sb.s_ninodes) {
	    if (set_bit(j, bh->b_data)) {	/* shouldn't happen */
		unmap_buffer(bh);
		s = "mni: already set\n";
		break;
	    }
	    mark_buffer_dirty(bh);
	    unmap_buffer(bh);
	    if (!k) {
		s = "new_inode: iput fail\n";
		break;
	    }
	    inode->i_ino = (ino_t)k;
	    inode->i_dirt = 1;

#ifdef BLOAT_FS
	    inode->i_blocks = inode->i_blksize = 0;
#endif

	    return inode;
	}
	unmap_buffer(bh);
    } while (++i < inode->i_sb->u.minix_sb.s_imap_blocks);
    printk(s);
    iput(inode);
    return NULL;
}
