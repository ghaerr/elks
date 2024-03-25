/*
 *  linux-0.97/fs/msdos/misc.c
 *
 *  Written 1992 by Werner Almesberger
 *
 * Aug 2023 Greg Haerr - Don't use L1 cache/memset on new filesystem clusters.
 */

#include <linuxmt/config.h>
#include <linuxmt/msdos_fs.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>
#include <linuxmt/debug.h>

struct buffer_head * FATPROC msdos_sread_nomap(struct super_block *s, sector_t sector,
                                                 size_t *offset)
{
    struct buffer_head *bh;

    if (!(bh = bread32(s->s_dev, sector >> (BLOCK_SIZE_BITS - SECTOR_BITS_SB(s)))))
        return NULL;

    //debug_fat("msread sector %ld block %lu\n", sector, buffer_blocknr(bh));
    *offset = ((int)sector & (BLOCK_SIZE_BITS - SECTOR_BITS_SB(s))) << SECTOR_BITS_SB(s);
    return bh;
}

struct buffer_head * FATPROC msdos_sread(struct super_block *s, sector_t sector, void **start)
{
    struct buffer_head *bh;
    size_t offset;

    if (!(bh = msdos_sread_nomap(s, sector, &offset)))
        return NULL;

    map_buffer(bh);
    *start = bh->b_data + offset;
    return bh;
}


static struct wait_queue creation_wait;
static int creation_lock = 0;


void FATPROC lock_creation(void)
{
	while (creation_lock) sleep_on(&creation_wait);
	creation_lock = 1;
}


void FATPROC unlock_creation(void)
{
	creation_lock = 0;
	wake_up(&creation_wait);
}


int FATPROC msdos_add_cluster(register struct inode *inode)
{
	static struct wait_queue wait;
	static int lock = 0;
	cluster_t count, this, limit, curr, last;
	sector_t sector;
	size_t offset;
	struct buffer_head *bh;
	struct msdos_sb_info *sb = MSDOS_SB(inode->i_sb);
	int fatsz = sb->fat_bits;
	cluster_t prev = sb->previous_cluster;

	debug_fat("add_cluster\n");
#ifndef FAT_BITS_32
	if (fatsz != 32)
		if (inode->i_ino == MSDOS_ROOT_INO) return -ENOSPC;
#endif
	while (lock) sleep_on(&wait);
	lock = 1;
	limit = sb->clusters;
	for (count = 0; count < limit; count++) {
		this = ((count+prev) % limit)+2;
		if (fat_access(inode->i_sb,this,-1L) == 0) break;
	}
	debug("free cluster: %d\r\n",this);

	prev = (count+prev+1) % limit;
	sb->previous_cluster = prev;
	if (count >= limit) {
		lock = 0;
		wake_up(&wait);
		return -ENOSPC;
	}
	fat_access(inode->i_sb,this,
#ifndef FAT_BITS_32
	    fatsz == 12? 0xff8UL : fatsz == 16? 0xfff8UL:
#endif
	    0xffffff8UL);
	lock = 0;
	wake_up(&wait);
	debug("set to %x\r\n",fat_access(inode->i_sb,this,-1L));

	if (!S_ISDIR(inode->i_mode)) {
		last = inode->i_size?
			get_cluster(inode,(inode->i_size-1) / SECTOR_SIZE(inode) / sb->cluster_size)
			: 0;
	} else {
		last = 0;
		if ((curr = inode->u.msdos_i.i_start) != 0) {
			cache_lookup(inode,0x7fffffffL,&last,&curr);
			while (curr && curr != -1)
				if (!(curr = fat_access(inode->i_sb, last = curr,-1L))) {
					printk("FAT: no EOF in file");
					return -ENOSPC;
				}
			}
	}
	debug("last = %d\r\n",last);

	if (last)
		fat_access(inode->i_sb,last,this);
	else {
		inode->u.msdos_i.i_start = this;
		inode->i_dirt = 1;
	}
	if (last) debug("next set to %d\r\n",fat_access(inode->i_sb,last,-1L));

	for (curr = 0; curr < sb->cluster_size; curr++) {
		sector = sb->data_start + (this - 2) * sb->cluster_size + curr;
		debug("zeroing sector %lu\r\n", sector);

		if (curr < sb->cluster_size-1 && !(sector & 1)) {
			if (!(bh = getblk32(inode->i_dev, sector >> 1)))
				printk("FAT: getblk fail\n");
			else {
                debug_blk("msdos_add_cluster1: block %ld uptodate %d\n",
                    EBH(bh)->b_blocknr, EBH(bh)->b_uptodate);
                zero_buffer(bh, 0, BLOCK_SIZE);
				mark_buffer_uptodate(bh, 1);
			}
			curr++;
		} else {
			if (!(bh = msdos_sread_nomap(inode->i_sb, sector, &offset)))
				printk("FAT: sread fail\n");
			else {
				debug_blk("msdos_add_cluster2: block %ld uptodate %d\n",
					EBH(bh)->b_blocknr, EBH(bh)->b_uptodate);
				zero_buffer(bh, offset, SECTOR_SIZE(inode));
			}
		}
		if (bh) {
			debug_fat("add_cluster block write %lu\n", buffer_blocknr(bh));
			mark_buffer_dirty(bh);
			brelse(bh);
		}
	}
	if (S_ISDIR(inode->i_mode)) {
		if (inode->i_size & (SECTOR_SIZE(inode)-1)) {
			printk("FAT: bad dir size\n");
			return -ENOSPC;
		}
		inode->i_size += (cluster_t)SECTOR_SIZE(inode)*sb->cluster_size;
		inode->i_dirt = 1;
		debug("size is %d now (%x)\r\n",inode->i_size,inode);
	}
	return 0;
}


/* Linear day numbers of the respective 1sts in non-leap years. */

                  /* JanFebMarApr May Jun Jul Aug Sep Oct Nov Dec */
static int day_n[] = { 0,31,59,90,120,151,181,212,243,273,304,334,0,0,0,0 };


/* Convert a MS-DOS time/date pair to a UNIX date (seconds since 1 1 70). */

long FATPROC date_dos2unix(unsigned short time,unsigned short date)
{
	int month = ((date >> 5) & 15)-1;
	int year = date >> 9;

	/* days since 1.1.70 plus 80's leap day */
	return (time & 31) * 2
		+ 60 * ((time >> 5) & 63)
		+ (time >> 11) * 3600L
		+ 86400L * ((date & 31) - 1
					+ day_n[month]
					+ (year / 4)
					+ year * 365
					- ((year & 3) == 0 && month < 2 ? 1 : 0)
					+ 3653)
		+ tz_offset * 3600L;
}


/* Convert linear UNIX date to a MS-DOS time/date pair. */

void FATPROC date_unix2dos(long unix_date,unsigned short *time, unsigned short *date)
{
	int day,year,nl_day,month;

	unix_date -= tz_offset * 3600L;     /* convert to localtime for FAT */
	*time = (short)(unix_date % 60)/2 +
			((short)((unix_date/60) % 60) << 5) +
			((short)((unix_date/3600) % 24) << 11);

	day = (short)(unix_date/86400L) - 3652;
	if (day < 0)		/* correct for dates earlier than 1980 */
		day = 0;
	year = day/365;
	if ((year+3)/4 + 365*year > day)
		year--;
	day -= (year+3)/4 + 365*year;
	if (day == 59 && !(year & 3)) {
		nl_day = day;
		month = 2;
	} else {
		nl_day = (year & 3) || day <= 59? day : day-1;
		for (month = 0; month < 12; month++)
			if (day_n[month] > nl_day)
				break;
	}

//printk("Date unix %lu day %d month %d year %d\n", unix_date, nl_day-day_n[month-1] + 1, month, year);

	*date = nl_day-day_n[month-1] + 1 + (month << 5) + (year << 9);
}


/* Returns the inode number of the directory entry at offset pos. If bh is
   non-NULL, it is brelse'd before. Pos is incremented. The buffer header is
   returned in bh. */

ino_t FATPROC msdos_get_entry(struct inode *dir,loff_t *pos,struct buffer_head **bh,
    struct msdos_dir_entry **de)
{
	sector_t sector;
	int offset;
	void *data;

#ifdef CONFIG_FS_DEV
	if (dir->i_ino == MSDOS_SB(dir->i_sb)->dev_ino) {
		unsigned i = (unsigned)*pos / sizeof(struct msdos_dir_entry);
		if (i - 2 <= DEVDIR_SIZE) {
			static struct msdos_dir_entry deventry;
			int j;

			i -= 2;
			*pos += sizeof(struct msdos_dir_entry);
			*de = &deventry;
			memset(deventry.name, ' ', MSDOS_NAME);
			for (j = strlen(devnods[i].name); --j >= 0; )
			    deventry.name[j] = devnods[i].name[j];

			i += DEVINO_BASE;
			return i;
	    }
	}
#endif

	while (1) {
		offset = *pos;
		if ((sector = msdos_smap(dir,(sector_t)(*pos >> SECTOR_BITS(dir)))) == -1)
			return -1;
		if (!sector)
			return -1; /* FAT error ... */
		*pos += sizeof(struct msdos_dir_entry);
		if (*bh)
			unmap_brelse(*bh);
		if (!(*bh = msdos_sread(dir->i_sb,sector,&data)))
			continue;
		*de = (struct msdos_dir_entry *) ((char *)data+(offset & (SECTOR_SIZE(dir)-1)));

		//debug_fat("get entry %lu\n", sector);
		return (sector << MSDOS_DPS_BITS(dir))+((offset & (SECTOR_SIZE(dir)-1)) >> MSDOS_DIR_BITS);
	}
}

/* Scans a directory for a given file (name points to its formatted name) or
   for an empty directory slot (name is NULL). Returns the inode number. */

int FATPROC msdos_scan(struct inode *dir,char *name,struct buffer_head **res_bh,
    struct msdos_dir_entry **res_de,ino_t *ino)
{
	off_t pos;
	struct msdos_dir_entry *de;
	struct inode *inode;

	pos = 0;
	*res_bh = NULL;
	while ((*ino = msdos_get_entry(dir,&pos,res_bh,&de)) != (ino_t)-1) {
		if (name) {
			if (de->name[0] && (unsigned char)de->name[0] != DELETED_FLAG
				&& !(de->attr & ATTR_VOLUME) && !strncmp(de->name,name,MSDOS_NAME)) break;
		}
		else if (!de->name[0] || (unsigned char)de->name[0] == DELETED_FLAG) {
				/* unset directory bit so read_inode doesn't traverse FAT table */
				/* MSDOS sometimes has deleted DIR entries with non-zero first cluster */
				de->attr &= ~ATTR_DIR;
				if (!(inode = iget(dir->i_sb,*ino)))
					break;
				if (!inode->u.msdos_i.i_busy) {
					iput(inode);
					break;
				}
				/* skip deleted files that haven't been closed yet */
				iput(inode);
		}
	}
	if (*ino == (ino_t)-1L) {
		if (*res_bh) unmap_brelse(*res_bh);
		*res_bh = NULL;
		return name ? -ENOENT : -ENOSPC;
	}
	*res_de = de;
	return 0;
}

/* Now an ugly part: this set of directory scan routines works on clusters
   rather than on inodes and sectors. They are necessary to locate the '..'
   directory "inode". */

/* Retrieve sectors sector */
static cluster_t FATPROC raw_found(struct super_block *sb, cluster_t sector,
	const char *name, cluster_t number, ino_t *ino)
{
	struct buffer_head *bh;
	struct msdos_dir_entry *data;
	int entry;
	cluster_t start = -1;

	if ((bh = msdos_sread(sb,sector,(void **) &data))) {
	  for (entry = 0; entry < MSDOS_DPS_SB(sb); entry++) {
#ifndef FAT_BITS_32
		unsigned short starthi = (MSDOS_SB(sb)->fat_bits == 32) ? data[entry].starthi : 0;
#else
		unsigned short starthi = data[entry].starthi;
#endif
		if (name) {
			if (strncmp(data[entry].name,name,MSDOS_NAME) != 0)
				continue;
			start = data[entry].start | ((unsigned long)starthi << 16);
	    } else {
			if ((unsigned char)data[entry].name[0] == DELETED_FLAG
				|| data[entry].start != (unsigned short)number
				|| starthi != (unsigned short)(number >> 16))
				continue;
			start = number;
	    }
		if (ino)
			*ino = sector*MSDOS_DPS_SB(sb) + entry;
		break;
	  }
	  unmap_brelse(bh);
	}
	return start;
}

/* Retrieve the root directory file */
static cluster_t FATPROC raw_scan_root(register struct super_block *sb,
	const char *name, cluster_t number,ino_t *ino)
{
	int count;
	cluster_t cluster = 0;

	for (count = 0; count < MSDOS_SB(sb)->dir_entries/MSDOS_DPS_SB(sb); count++) {
		if ((cluster = raw_found(sb,(cluster_t)(MSDOS_SB(sb)->dir_start+count),name, number,
				ino)) >= 0) break;
	}
	return cluster;
}

/* Retrieve the normal directory file */
static cluster_t FATPROC raw_scan_nonroot(register struct super_block *sb,
	cluster_t start, const char *name, cluster_t number, ino_t *ino)
{
	int count;
	cluster_t cluster;

	do {
		for (count = 0; count < MSDOS_SB(sb)->cluster_size; count++) {
			if ((cluster = raw_found(sb,
				(start-2)*MSDOS_SB(sb)->cluster_size+MSDOS_SB(sb)->data_start+count,
				name, number,ino)) >= 0) return cluster;
		}
		if (!(start = fat_access(sb,start,-1L))) {
			printk("FAT: bad fat");
			return -1;
		}
	} while (start != -1);
	return -1;
}

/* In the directory file (cluster start) within the name or cluster number number
 * to retrieve the file, return to its ino and cluster number
 */
static cluster_t FATPROC raw_scan(struct super_block *sb, cluster_t start,
	const char *name, cluster_t number, ino_t *ino)
{
    if (start)
		return raw_scan_nonroot(sb,start,name,number,ino);
    return raw_scan_root(sb,name,number,ino);
}

ino_t FATPROC msdos_parent_ino(register struct inode *dir,int locked)
{
	cluster_t curr,prev;
	ino_t this = (ino_t)-1L;

	if (!S_ISDIR(dir->i_mode))	/* actually coding error if occurs*/
		return (ino_t)-1L;
	if (dir->i_ino == MSDOS_ROOT_INO) return dir->i_ino;
	if (!locked) lock_creation(); /* prevent renames */
	if ((curr = raw_scan(dir->i_sb,dir->u.msdos_i.i_start,MSDOS_DOTDOT,0L, NULL)) < 0) {
	} else if (!curr) this = MSDOS_ROOT_INO;
	else {
		if ((prev = raw_scan(dir->i_sb,curr,MSDOS_DOTDOT,0L,NULL)) < 0) {
			if (MSDOS_SB(dir->i_sb)->fat_bits == 32 &&
			    curr == MSDOS_SB(dir->i_sb)->root_cluster)
					this = MSDOS_ROOT_INO;
		} else {
			if (prev == 0 
#ifndef FAT_BITS_32
				&& MSDOS_SB(dir->i_sb)->fat_bits == 32
#endif
		    )
				prev = MSDOS_SB(dir->i_sb)->root_cluster;
			raw_scan(dir->i_sb,prev,NULL,curr,&this);
		}
	}
	if (!locked) unlock_creation();
	return this;
}
