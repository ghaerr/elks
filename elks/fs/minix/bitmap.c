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

/* Static functions in this file */

static unsigned short count_used(struct buffer_head **,unsigned int,
				 unsigned int);

/* Function definitions */

static char nibblemap[] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };

static unsigned short count_used(struct buffer_head **map,
				 unsigned int numblocks,
				 unsigned int numbits)
{
    register struct buffer_head *bh;
    unsigned int end, i, j, sum = 0;

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
    return (sum);
}

unsigned short minix_count_free_blocks(register struct super_block *sb)
{
    struct minix_sb_info ms = sb->u.minix_sb;

    return (ms.s_nzones - count_used(ms.s_zmap, ms.s_zmap_blocks, ms.s_nzones))
			<< ms.s_log_zone_size;
}


unsigned short minix_count_free_inodes(register struct super_block *sb)
{
    struct minix_sb_info ms = sb->u.minix_sb;

    return ms.s_ninodes - count_used(ms.s_imap, ms.s_imap_blocks, ms.s_ninodes);
}

#endif

void minix_free_block(register struct super_block *sb, block_t block)
{
    register struct buffer_head *bh;
    register struct minix_sb_info *ms;
    unsigned int zone;
    unsigned short int bit;

    if (!sb) {
	printk("mfb: bad dev\n");
	return;
    }
    ms = &sb->u.minix_sb;
    if (block < ms->s_firstdatazone || block >= ms->s_nzones) {
	printk("trying to free block %lx not in datazone\n", block);
	return;
    }
    bh = get_hash_table(sb->s_dev, block);
    if (bh)
	bh->b_dirty = 0;
    brelse(bh);
    zone = block - ms->s_firstdatazone + 1;
    bit = (unsigned short int) (zone & 8191);
    zone >>= 13;
    bh = ms->s_zmap[zone];
    if (!bh) {
	printk("mfb: bad bitbuf\n");
	return;
    }
    map_buffer(bh);
    if (!clear_bit(bit, bh->b_data))
	printk("mfb (%s:%ld): already cleared\n", kdevname(sb->s_dev), block);
    unmap_buffer(bh);
    mark_buffer_dirty(bh, 1);
    return;
}

block_t minix_new_block(register struct super_block *sb)
{
    register struct buffer_head *bh;
    register struct minix_sb_info *ms;
    block_t i, j;

    if (!sb) {
	printk("mnb: no sb\n");
	return 0;
    }
    ms = &sb->u.minix_sb;

  repeat:
    j = 8192;
    for (i = 0; i < 8; i++)
	if ((bh = ms->s_zmap[i]) != NULL) {
	    map_buffer(bh);
	    if ((j = find_first_zero_bit(bh->b_data, 8192)) < 8192)
		break;
	    unmap_buffer(bh);
	}
    if (i >= 8 || !bh || j >= 8192)
	return 0;
    if (set_bit(j, bh->b_data)) {
	panic("mnb: already set %d %d\n", j, bh->b_data);
	unmap_buffer(bh);
	goto repeat;
    }
    if (j == find_first_zero_bit(bh->b_data, 8192))
	panic("still zero bit!%d\n", j);
    unmap_buffer(bh);
    mark_buffer_dirty(bh, 1);
    j += i * 8192 + ms->s_firstdatazone - 1;
    if (j < ms->s_firstdatazone || j >= ms->s_nzones)
	return 0;

    /* WARNING: j is not converted, so we have to do it */
    if (!(bh = getblk(sb->s_dev, j))) {
	printk("mnb: cannot get");
	return 0;
    }
    map_buffer(bh);
    memset(bh->b_data, 0, BLOCK_SIZE);
    mark_buffer_uptodate(bh, 1);
    mark_buffer_dirty(bh, 1);
    unmap_brelse(bh);
    return j;
}

static char *fi_name = "free_inode";

void minix_free_inode(register struct inode *inode)
{
    register struct buffer_head *bh;
    register struct minix_sb_info *ms;
    ino_t ino;

    if (!inode)
	return;
    if (!inode->i_dev) {
	printk("%s: no dev\n", fi_name);
	return;
    }
    if (inode->i_count != 1) {
	printk("%s: count=%d\n", fi_name, inode->i_count);
	return;
    }
    if (inode->i_nlink) {
	printk("%s: nlink=%d\n", fi_name, inode->i_nlink);
	return;
    }
    if (!inode->i_sb) {
	printk("%s: no sb\n", fi_name);
	return;
    }
    ino = inode->i_ino;
    ms = &inode->i_sb->u.minix_sb;
    if (ino < 1 || ino > ms->s_ninodes) {
	printk("%s: 0 or nonexistent\n", fi_name);
	return;
    }
    if (!(bh = ms->s_imap[ino >> 13])) {
	printk("%s: nonexistent imap\n", fi_name);
	return;
    }
    map_buffer(bh);
    clear_inode(inode);
    if (!clear_bit(ino & 8191, bh->b_data)) {
	debug2("%s: bit %ld already cleared.\n",fi_name,ino);
    }
    mark_buffer_dirty(bh, 1);
    unmap_buffer(bh);
}

struct inode *minix_new_inode(struct inode *dir)
{
    register struct buffer_head *bh;
    register struct inode *inode;
    register struct minix_sb_info *ms;

    /* Adding an sb here does not make the code smaller */
    unsigned short int i, j;

    if (!dir || !(inode = get_empty_inode()))
	return NULL;
    inode->i_sb = dir->i_sb;
    inode->i_flags = inode->i_sb->s_flags;
    j = 8192;
    ms = &inode->i_sb->u.minix_sb;
    for (i = 0; i < 8; i++)
	if ((bh = ms->s_imap[i]) != NULL) {
	    map_buffer(bh);
	    if ((j = find_first_zero_bit(bh->b_data, 8192)) < 8192)
		break;
	    unmap_buffer(bh);
	}
    if (!bh || j >= 8192) {
	printk("No new inodes found!\n");
	goto iputfail;
    }
    if (set_bit(j, bh->b_data)) {		/* shouldn't happen */
	printk("mni: already set\n");
	goto iputfail;
    }
    mark_buffer_dirty(bh, 1);
    j += i * 8192;
    if (!j || j > ms->s_ninodes) {
	goto iputfail;
    }
    unmap_buffer(bh);
    inode->i_count = 1;
    inode->i_nlink = 1;
    inode->i_dev = inode->i_sb->s_dev;
    inode->i_uid = current->euid;
    inode->i_gid = (__u8) ((dir->i_mode & S_ISGID) ? ((gid_t) dir->i_gid)
						   : current->egid);
    inode->i_dirt = 1;
    inode->i_ino = j;
    inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
    inode->i_op = NULL;

#ifdef BLOAT_FS
    inode->i_blocks = inode->i_blksize = 0;
#endif

    return inode;

    /*  Oh no! It's 'Return of Goto' in a double feature with
     *  'Mozilla vs. Internet Exploder' :)
     */

  iputfail:
    printk("new_inode: iput fail\n");
    unmap_buffer(bh);
    iput(inode);
    return NULL;
}
