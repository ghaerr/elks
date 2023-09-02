/*
 *  linux/fs/truncate.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 */

#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/minix_fs.h>
#include <linuxmt/stat.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/debug.h>

#define DIRECT_BLOCK		((inode->i_size + 1023) >> 10)
#define INDIRECT_BLOCK(offset)	((long)(DIRECT_BLOCK)-offset)
#define DINDIRECT_BLOCK(offset) (((long)(DIRECT_BLOCK)-offset)>>9)

/*
 * Truncate has the most races in the whole filesystem: coding it is
 * a pain in the a**. Especially as I don't do any locking...
 *
 * The code may look a bit weird, but that's just because I've tried to
 * handle things like file-size changes in a somewhat graceful manner.
 * Anyway, truncating a file at the same time somebody else writes to it
 * is likely to result in pretty weird behaviour...
 *
 * The new code handles normal truncates (size = 0) as well as the more
 * general case (size = XXX). I hope.
 */

/*
 * The functions for minix V1 fs truncation.
 */
static int V1_trunc_direct(register struct inode *inode)
{
    unsigned short *p;
    register struct buffer_head *bh;
    unsigned short tmp;
    unsigned int i;
    int retry = 0;

  repeat:
    debug("DIRECT_BLOCK %ld\n", DIRECT_BLOCK);
    for (i = DIRECT_BLOCK; i < 7; i++) {
	p = &inode->u.minix_i.i_zone[i];
	if (!(tmp = *p)) continue;
	bh = get_hash_table(inode->i_dev, (block_t) tmp);
	if (i < (unsigned int)DIRECT_BLOCK) {
	    brelse(bh);
	    goto repeat;
	}
	if ((bh && buffer_count(bh) != 1) || tmp != *p) {
	    retry = 1;
	    brelse(bh);
	    continue;
	}
	*p = 0;
	inode->i_dirt = 1;
	if (bh) {
	    mark_buffer_clean(bh);
	    brelse(bh);
	}
	minix_free_block(inode->i_sb, tmp);
    }
    return retry;
}

static int V1_trunc_indirect(register struct inode *inode,
			     unsigned int offset, unsigned short *p)
{
    struct buffer_head *bh;
    int i;
    long j;
    unsigned short tmp;
    register struct buffer_head *ind_bh;
    unsigned short *ind;
    int retry = 0;

    tmp = *p;
    if (!tmp) return 0;
    ind_bh = bread(inode->i_dev, (block_t) tmp);
    if (tmp != *p) {
	brelse(ind_bh);
	return 1;
    }
    if (!ind_bh) {
	*p = 0;
	return 0;
    }
    map_buffer(ind_bh);
  repeat:
    debug("INDIRECT_BLOCK %ld\n", INDIRECT_BLOCK(offset));
    for (j = INDIRECT_BLOCK(offset); j < 512; j++) {
	if (j < 0) j = 0;
	else if (j < INDIRECT_BLOCK(offset)) goto repeat;
	ind = (int)j + (unsigned short *) ind_bh->b_data;
	tmp = *ind;
	if (!tmp) continue;
	bh = get_hash_table(inode->i_dev, (block_t) tmp);
	if (j < INDIRECT_BLOCK(offset)) {
	    brelse(bh);
	    goto repeat;
	}
	if ((bh && buffer_count(bh) != 1) || tmp != *ind) {
	    retry = 1;
	    brelse(bh);
	    continue;
	}
	*ind = 0;
	mark_buffer_dirty(ind_bh);
	brelse(bh);
	minix_free_block(inode->i_sb, tmp);
    }
    ind = (unsigned short *) ind_bh->b_data;
    for (i = 0; i < 512; i++)
	if (*(ind++)) break;
    if (i >= 512) {
	if (buffer_count(ind_bh) != 1) retry = 1;
	else {
	    tmp = *p;
	    *p = 0;
	    minix_free_block(inode->i_sb, tmp);
	}
    }
    unmap_brelse(ind_bh);
    return retry;
}

static int V1_trunc_dindirect(register struct inode *inode,
			      unsigned int offset, unsigned short *p)
{
    int i;
    unsigned short tmp;
    register struct buffer_head *dind_bh;
    unsigned short *dind;
    int retry = 0;

    if (!(tmp = *p)) return 0;
    dind_bh = bread(inode->i_dev, (block_t) tmp);
    if (tmp != *p) {
	brelse(dind_bh);
	return 1;
    }
    if (!dind_bh) {
	*p = 0;
	return 0;
    }
    map_buffer(dind_bh);
  repeat:
    debug("DINDIRECT_BLOCK %ld\n", DINDIRECT_BLOCK(offset));
    for (i = DINDIRECT_BLOCK(offset); i < 512; i++) {
	if (i < 0) i = 0;
	if (i < DINDIRECT_BLOCK(offset)) goto repeat;
	dind = i + (unsigned short *) dind_bh->b_data;
	retry |= V1_trunc_indirect(inode, offset + (i << 9), dind);
	mark_buffer_dirty(dind_bh);
    }
    dind = (unsigned short *) dind_bh->b_data;
    for (i = 0; i < 512; i++)
	if (*(dind++)) break;
    if (i >= 512) {
	if (buffer_count(dind_bh) != 1) retry = 1;
	else {
	    tmp = *p;
	    *p = 0;
	    inode->i_dirt = 1;
	    minix_free_block(inode->i_sb, tmp);
	}
    }
    unmap_brelse(dind_bh);
    return retry;
}

void minix_truncate(register struct inode *inode)
{
    int retry;

    if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) ||
	  S_ISLNK(inode->i_mode))) return;
    while (1) {
	retry = V1_trunc_direct(inode);
	retry |= V1_trunc_indirect(inode, 7, &inode->u.minix_i.i_zone[7]);
	retry |= V1_trunc_dindirect(inode, 7 + 512, &inode->u.minix_i.i_zone[8]);
	if (!retry) break;
	schedule();
    }
    inode->i_mtime = inode->i_ctime = current_time();
    inode->i_dirt = 1;
}
