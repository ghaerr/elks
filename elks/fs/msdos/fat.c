/*
 *  linux/fs/msdos/fat.c
 *
 *  Written 1992 by Werner Almesberger
 */

#include <linuxmt/msdos_fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/stat.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>

static struct fat_cache *fat_cache,cache[FAT_CACHE];

/* Returns the this'th FAT entry, -1 if it is an end-of-file entry.
   If new_value is != -1, that FAT entry is replaced by it. */

cluster_t FATPROC fat_access(struct super_block *sb,cluster_t this,cluster_t new_value)
{
	struct buffer_head *bh,*bh2;
	unsigned char *p_first,*p_last;
	void *data,*data2;
	cluster_t first,last,next;
	int fatsz = MSDOS_SB(sb)->fat_bits;
	//long copy;
	//void *c_data, *c_data2;
	//struct buffer_head *c_bh, *c_bh2;

#ifndef FAT_BITS_32
	if (fatsz == 32)
#endif
		first = last = this << 2;	/* = 4x*/
#ifndef FAT_BITS_32
	else if (fatsz == 16)
		first = last = this << 1;	/* = 2x*/
	else {	/* fat12*/
		first = this + (this >> 1);	/* = 3/2: no multiply for speed*/
		last = first+1;
	}
#endif
	if (!(bh = msdos_sread(sb,
			(sector_t)(MSDOS_SB(sb)->fat_start+(first >> SECTOR_BITS_SB(sb))),&data))) {
		printk("FAT: bread fat failed\n");
		return 0;
	}
	if ((first >> SECTOR_BITS_SB(sb)) == (last >> SECTOR_BITS_SB(sb))) {
		bh2 = bh;
		data2 = data;
	} else {	/* FAT entry crosses sector boundary, only possible with fat12*/
		if (!(bh2 = msdos_sread(sb,
			(sector_t)(MSDOS_SB(sb)->fat_start+(last >> SECTOR_BITS_SB(sb))),&data2))) {
			unmap_brelse(bh);
			printk("FAT: bread fat failed\n");
			return 0;
		}
	}
#ifndef FAT_BITS_32
	if (fatsz == 32)
#endif
	{
		next = ((unsigned long *)data)[(first & (SECTOR_SIZE_SB(sb)-1)) >> 2];
		if (next >= 0xffffff8L) next = -1;
	}
#ifndef FAT_BITS_32
	else if (fatsz == 16) {
		next = ((unsigned short *)data)[(first & (SECTOR_SIZE_SB(sb)-1)) >> 1];
		if (next >= 0xfff8) next = -1;
	} else {	/* fat12*/
		p_first = &((unsigned char *)data)[first & (SECTOR_SIZE_SB(sb)-1)];
		p_last = &((unsigned char *)data2)[(first+1) & (SECTOR_SIZE_SB(sb)-1)];
		if (this & 1) next = ((*p_first >> 4) | (*p_last << 4)) & 0xfff;
		else next = (*p_first+(*p_last << 8)) & 0xfff;
		if (next >= 0xff8) next = -1;
	}
#endif
	if (new_value != -1) {
#ifndef FAT_BITS_32
		if (fatsz == 32)
#endif
			((unsigned long *)data)[(first & (SECTOR_SIZE_SB(sb)-1))>>2] =
				(unsigned long)new_value;
#ifndef FAT_BITS_32
		else if (fatsz == 16)
			((unsigned short *)data)[(first & (SECTOR_SIZE_SB(sb)-1)) >> 1] =
				(unsigned short)new_value;
		else {	/* fat12*/
			if (this & 1) {
				*p_first = (*p_first & 0xf) | (new_value << 4);
				*p_last = new_value >> 4;
			}
			else {
				*p_first = new_value & 0xff;
				*p_last = (*p_last & 0xf0) | (new_value >> 8);
			}
			if (bh != bh2) debug_fat("fat_access block bh2 dirty %lu\n", buffer_blocknr(bh2));
			mark_buffer_dirty(bh2);
		}
#endif
		debug_fat("fat_access block bh dirty %lu\n", buffer_blocknr(bh));
		mark_buffer_dirty(bh);
#if UNUSED  /* FIXME update 2nd FAT table? */
		/* update extra FAT table copies*/
		//FIXME does not look like multiple fat table update is working
		// should use copy=2 and <= #fats below
		// temp removed because of useless slow memcpy below w/each file write
		// needs rewrite for speed using operations above then mark dirty
		// or perhaps just make a complete FAT table copy on unmount!
		for (copy = 1; copy < MSDOS_SB(sb)->fats; copy++) {
			if (!(c_bh = msdos_sread(sb,
				(sector_t)(MSDOS_SB(sb)->fat_start+(first >> SECTOR_BITS_SB(sb)) +
				MSDOS_SB(sb)->fat_length*copy), &c_data)))
				break;
			memcpy(c_data,data,SECTOR_SIZE_SB(sb));
		debug_fat("fat_access block cbh write %lu\n", buffer_blocknr(bh));
			mark_buffer_dirty(c_bh);
			if (data != data2 || bh != bh2) {
				if (!(c_bh2 = msdos_sread(sb,
					(sector_t)(MSDOS_SB(sb)->fat_start+(first >> SECTOR_BITS_SB(sb)) +
					MSDOS_SB(sb)->fat_length*copy + 1), &c_data2))) {
					unmap_brelse(c_bh);
					break;
				}
				memcpy(c_data2,data2,SECTOR_SIZE_SB(sb));
				mark_buffer_dirty(c_bh2);	//FIXME looks like bug without this
		debug_fat("fat_access block cbh2 write %lu\n", buffer_blocknr(c_bh2));
				unmap_brelse(c_bh2);
			}
			unmap_brelse(c_bh);
		}
#endif /* extra copy FAT table update*/
	}
	unmap_brelse(bh);
	if (data != data2) unmap_brelse(bh2);
	return next;
}


void FATPROC cache_init(void)
{
	static int initialized = 0;
	int count;

	if (initialized) return;
	fat_cache = &cache[0];
	for (count = 0; count < FAT_CACHE; count++) {
		cache[count].device = 0;
		cache[count].next = count == (FAT_CACHE - 1)? NULL : &cache[count+1];
	}
	initialized = 1;
}


void FATPROC cache_lookup(struct inode *inode,cluster_t cluster,
	cluster_t *f_clu, cluster_t *d_clu)
{
	register struct fat_cache *walk;

	debug("cache lookup: %d\r\n",*f_clu);

	for (walk = fat_cache; walk; walk = walk->next)
		if (inode->i_dev == walk->device && walk->ino == inode->i_ino
			&& walk->file_cluster <= cluster && walk->file_cluster > *f_clu) {
			*d_clu = walk->disk_cluster;
			debug("cache hit: %ld (%ld)\r\n",walk->file_cluster,*d_clu);

			if ((*f_clu = walk->file_cluster) == cluster) return;
		}
}


#if DEBUG
static void FATPROC list_cache(void)
{
	struct fat_cache *walk;

	for (walk = fat_cache; walk; walk = walk->next) {
		if (walk->device) dprintk("(%d,%d) ",walk->file_cluster,
			walk->disk_cluster);
		else dprintk("-- ");
	}
	dprintk("\r\n");
}
#endif


void FATPROC cache_add(struct inode *inode, cluster_t f_clu, cluster_t d_clu)
{
	register struct fat_cache *walk,*last;

	debug("cache add: %d (%d)\r\n",f_clu,d_clu);

	last = NULL;
	for (walk = fat_cache; walk->next; walk = (last = walk)->next)
		if (inode->i_dev == walk->device && walk->ino == inode->i_ino
			&& walk->file_cluster == f_clu) {
			if (walk->disk_cluster != d_clu) {
				printk("FAT: corrupt cache");
				return;
			}
			/* update LRU */
			if (last == NULL) return;
			last->next = walk->next;
			walk->next = fat_cache;
			fat_cache = walk;
#if DEBUG
			list_cache();
#endif
			return;
		}
	walk->device = inode->i_dev;
	walk->ino = inode->i_ino;
	walk->file_cluster = f_clu;
	walk->disk_cluster = d_clu;
	last->next = NULL;
	walk->next = fat_cache;
	fat_cache = walk;
#if DEBUG
	list_cache();
#endif
}


/* Cache invalidation occurs rarely, thus the LRU chain is not updated. It
   fixes itself after a while. */

void FATPROC cache_inval_inode(struct inode *inode)
{
	register struct fat_cache *walk;

	for (walk = fat_cache; walk; walk = walk->next)
		if (walk->device == inode->i_dev && walk->ino == inode->i_ino)
			walk->device = 0;
}


void FATPROC cache_inval_dev(kdev_t device)
{
	register struct fat_cache *walk;

	for (walk = fat_cache; walk; walk = walk->next)
		if (walk->device == device) walk->device = 0;
}


cluster_t FATPROC get_cluster(register struct inode *inode, cluster_t cluster)
{
	cluster_t this, count;

	if (!(this = inode->u.msdos_i.i_start)) return 0;
	if (!cluster) return this;
	count = 0;
	for (cache_lookup(inode,cluster,&count,&this); count < cluster; count++) {
		if ((this = fat_access(inode->i_sb,this,-1L)) == -1) return 0;
		if (!this) return 0;
	}
	cache_add(inode,cluster,this);
	return this;
}


sector_t FATPROC msdos_smap(struct inode *inode, sector_t sector)
{
	register struct msdos_sb_info *sb;
	cluster_t cluster;
	int offset;

	sb = MSDOS_SB(inode->i_sb);
#ifndef FAT_BITS_32
	if ((sb->fat_bits != 32)
		&& (inode->i_ino == MSDOS_ROOT_INO
		    || (S_ISDIR(inode->i_mode) && !inode->u.msdos_i.i_start))) {
		if (sector >= sb->dir_entries >> MSDOS_DPS_BITS(inode)) return 0;
		return sector+sb->dir_start;
	}
#endif
	cluster = sector / sb->cluster_size;
	offset = (unsigned int)sector % sb->cluster_size;
	if (!(cluster = get_cluster(inode,cluster))) return 0;
	return (cluster-2)*sb->cluster_size+sb->data_start+offset;
}


/* Free all clusters after the skip'th cluster. Doesn't use the cache,
   because this way we get an additional sanity check. */

int FATPROC fat_free(register struct inode *inode, cluster_t skip)
{
	cluster_t this, last;

	debug_fat("fat_free\n");
	if (!(this = inode->u.msdos_i.i_start)) return 0;
	last = 0;
	while (skip--) {
		last = this;
		if ((this = fat_access(inode->i_sb,this,-1L)) == -1)
			return 0;
		if (!this) {
			printk("FAT: missing EOF\n");
			return -EIO;
		}
	}
	if (last)
		fat_access(inode->i_sb,last,
#ifndef FAT_BITS_32
		MSDOS_SB(inode->i_sb)->fat_bits == 12 ? 
		0xff8UL : MSDOS_SB(inode->i_sb)->fat_bits==16 ? 0xfff8UL : 
#endif
		0xffffff8UL);
	else {
		inode->u.msdos_i.i_start = 0;
		inode->i_dirt = 1;
	}
	while (this != -1)
		if (!(this = fat_access(inode->i_sb,this,0L))) {
			printk("FAT: delete past EOF");
			return -EIO;
		}
	cache_inval_inode(inode);
	return 0;
}
