/*
 *  linux-0.97/fs/msdos/misc.c
 *
 *  Written 1992 by Werner Almesberger
 */

#include <linuxmt/msdos_fs.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>
#include <linuxmt/debug.h>

struct buffer_head *msdos_sread(int dev,long sector,void **start)
{
	register struct buffer_head *bh;

	if (!(bh = bread(dev, (block_t)(sector >> 1) )))
		return NULL;

	map_buffer(bh);
	//debug_fat("msread sector %ld block %d\n", sector, bh->b_blocknr);
	*start = bh->b_data + (((int)sector & 1) << SECTOR_BITS);
	return bh;
}


static struct wait_queue *creation_wait = NULL;
static int creation_lock = 0;


void lock_creation(void)
{
	while (creation_lock) sleep_on(&creation_wait);
	creation_lock = 1;
}


void unlock_creation(void)
{
	creation_lock = 0;
	wake_up(&creation_wait);
}


int msdos_add_cluster(register struct inode *inode)
{
	static struct wait_queue *wait = NULL;
	static int lock = 0;
	//FIXME: using previous on booted FAT volume with mounted FAT floppy won't work well
	static long previous = 0; /* works best if one FS is being used */
	long count,this,limit,last,current,sector;
	void *data;
	struct buffer_head *bh;
	int fatsz = MSDOS_SB(inode->i_sb)->fat_bits;

	debug_fat("add_cluster\n");
#ifndef FAT_BITS_32
	if (fatsz != 32)
		if (inode->i_ino == MSDOS_ROOT_INO) return -ENOSPC;
#endif
	while (lock) sleep_on(&wait);
	lock = 1;
	limit = MSDOS_SB(inode->i_sb)->clusters;
	//this = limit; /* to keep GCC happy */
	for (count = 0; count < limit; count++) {
		this = ((count+previous) % limit)+2;
		if (fat_access(inode->i_sb,this,-1L) == 0) break;
	}
#ifdef DEBUG
printk("free cluster: %d\r\n",this);
#endif
	previous = (count+previous+1) % limit;
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
#ifdef DEBUG
printk("set to %x\r\n",fat_access(inode->i_sb,this,-1L));
#endif
	if (!S_ISDIR(inode->i_mode)) {
		last = inode->i_size?
			get_cluster(inode,(inode->i_size-1) / SECTOR_SIZE /
			MSDOS_SB(inode->i_sb)->cluster_size) : 0;
	} else {
		last = 0;
		if ((current = inode->u.msdos_i.i_start) != 0) {
			cache_lookup(inode,0x7fffffffL,&last,&current);
			while (current && current != -1)
				if (!(current = fat_access(inode->i_sb, last = current,-1L))) {
					printk("FAT: no EOF in file");
					return -ENOSPC;
				}
			}
	}
#ifdef DEBUG
printk("last = %d\r\n",last);
#endif
	if (last)
		fat_access(inode->i_sb,last,this);
	else {
		inode->u.msdos_i.i_start = this;
		inode->i_dirt = 1;
	}
#ifdef DEBUG
if (last) printk("next set to %d\r\n",fat_access(inode->i_sb,last,-1L));
#endif
	for (current = 0; current < MSDOS_SB(inode->i_sb)->cluster_size; current++) {
		sector = MSDOS_SB(inode->i_sb)->data_start+(this-2) *
			MSDOS_SB(inode->i_sb)->cluster_size+current;
#ifdef DEBUG
printk("zeroing sector %d\r\n",sector);
#endif
		if (current < MSDOS_SB(inode->i_sb)->cluster_size-1 && !(sector & 1)) {
			if (!(bh = getblk(inode->i_dev,(block_t)(sector >> 1))))
				printk("FAT: getblk fail\n");
			else {
				map_buffer(bh);
				memset(bh->b_data,0,BLOCK_SIZE);
				bh->b_uptodate = 1;
			}
			current++;
		} else {
			if (!(bh = msdos_sread(inode->i_dev,sector,&data)))
				printk("FAT: sread fail\n");
			else memset(data,0,SECTOR_SIZE);
		}
		if (bh) {
			debug_fat("add_cluster block write %d\n", bh->b_blocknr);
			bh->b_dirty = 1;
			unmap_brelse(bh);
		}
	}
	if (S_ISDIR(inode->i_mode)) {
		if (inode->i_size & (SECTOR_SIZE-1)) {
			printk("FAT: bad dir size\n");
			return -ENOSPC;
		}
		inode->i_size += SECTOR_SIZE*MSDOS_SB(inode->i_sb)->cluster_size;
#ifdef DEBUG
printk("size is %d now (%x)\r\n",inode->i_size,inode);
#endif
		inode->i_dirt = 1;
	}
	return 0;
}


/* Linear day numbers of the respective 1sts in non-leap years. */

                  /* JanFebMarApr May Jun Jul Aug Sep Oct Nov Dec */
static int day_n[] = { 0,31,59,90,120,151,181,212,243,273,304,334,0,0,0,0 };


/* Convert a MS-DOS time/date pair to a UNIX date (seconds since 1 1 70). */

long date_dos2unix(unsigned short time,unsigned short date)
{
	int month,year;

	month = ((date >> 5) & 15)-1;
	year = date >> 9;
	return (time & 31)*2+60*((time >> 5) & 63)+(time >> 11)*3600L+86400L*
	    ((date & 31)-1+day_n[month]+(year/4)+year*365-((year & 3) == 0 &&
	    month < 2 ? 1 : 0)+3653);
			/* days since 1.1.70 plus 80's leap day */
}


/* Convert linear UNIX date to a MS-DOS time/date pair. */

void date_unix2dos(long unix_date,unsigned short *time, unsigned short *date)
{
	int day,year,nl_day,month;

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

ino_t msdos_get_entry(struct inode *dir,loff_t *pos,struct buffer_head **bh,
    struct msdos_dir_entry **de)
{
	long sector;
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
			    deventry.name[j] = toupper(devnods[i].name[j]);

			i += DEVINO_BASE;
			return i;
	    }
	}
#endif

	while (1) {
		offset = *pos;
		if ((sector = msdos_smap(dir,(long)(*pos >> SECTOR_BITS))) == -1)
			return -1;
		if (!sector)
			return -1; /* FAT error ... */
		*pos += sizeof(struct msdos_dir_entry);
		if (*bh)
			unmap_brelse(*bh);
		if (!(*bh = msdos_sread(dir->i_dev,sector,&data)))
			continue;
		*de = (struct msdos_dir_entry *) ((char *)data+(offset & (SECTOR_SIZE-1)));

		/* return value will overfow for FAT16/32 if sector is beyond 2MB boundary */
#ifndef CONFIG_32BIT_INODES
		if (sector > 4095) {
			printk("FAT: disk too large, set CONFIG_32BIT_INODES\n");
			return -1;
		}
#endif
		//debug_fat("get entry %ld\n", sector);
		return (sector << MSDOS_DPS_BITS)+((offset & (SECTOR_SIZE-1)) >> MSDOS_DIR_BITS);
	}
}

/* Scans a directory for a given file (name points to its formatted name) or
   for an empty directory slot (name is NULL). Returns the inode number. */

int msdos_scan(struct inode *dir,char *name,struct buffer_head **res_bh,
    struct msdos_dir_entry **res_de,ino_t *ino)
{
	off_t pos;
	struct msdos_dir_entry *de;
	struct inode *inode;

	pos = 0;
	*res_bh = NULL;
	while ((*ino = msdos_get_entry(dir,&pos,res_bh,&de)) != -1) {
		if (name) {
			if (de->name[0] && ((unsigned char *) (de->name))[0] != DELETED_FLAG
				&& !(de->attr & ATTR_VOLUME) && !strncmp(de->name,name,MSDOS_NAME)) break;
		}
		else if (!de->name[0] || ((unsigned char *) (de->name))[0] == DELETED_FLAG) {
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
static long raw_found(struct super_block *sb,long sector,char *name,long number,
    ino_t *ino)
{
	struct buffer_head *bh;
	struct msdos_dir_entry *data;
	int entry;
	long start = -1;

	if ((bh = msdos_sread(sb->s_dev,sector,(void **) &data))) {
	  for (entry = 0; entry < MSDOS_DPS; entry++) {
#ifndef FAT_BITS_32
		unsigned short starthi = (MSDOS_SB(sb)->fat_bits == 32) ? data[entry].starthi : 0;
#else
		unsigned short starthi = data[entry].starthi;
#endif
		if (name) {
			if (strncmp(data[entry].name,name,MSDOS_NAME) != 0)
				continue;
			((unsigned short *)&start)[0] = data[entry].start;
			((unsigned short *)&start)[1] = starthi;
	    } else {
			if (*(unsigned char *)data[entry].name == DELETED_FLAG
				|| data[entry].start != (unsigned short)number
				|| starthi != ((unsigned short *)&number)[1])
				continue;
			start = number;
	    }
		if (ino)
			*ino = sector*MSDOS_DPS + entry;
		break;
	  }
	  unmap_brelse(bh);
	}
	return start;
}

/* Retrieve the root directory file */
static long raw_scan_root(register struct super_block *sb,char *name,long number,ino_t *ino)
{
	int count;
	long cluster = 0;

	for (count = 0; count < MSDOS_SB(sb)->dir_entries/MSDOS_DPS; count++) {
		if ((cluster = raw_found(sb,(long)(MSDOS_SB(sb)->dir_start+count),name, number,
				ino)) >= 0) break;
	}
	return cluster;
}

/* Retrieve the normal directory file */
static long raw_scan_nonroot(register struct super_block *sb,long start,char *name,
    long number,ino_t *ino)
{
	int count;
	long cluster;

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
static long raw_scan(struct super_block *sb,long start,char *name,long number, ino_t *ino)
{
    if (start)
		return raw_scan_nonroot(sb,start,name,number,ino);
    return raw_scan_root(sb,name,number,ino);
}

ino_t msdos_parent_ino(register struct inode *dir,int locked)
{
	long current,prev;
	ino_t this = (ino_t)-1L;

	if (!S_ISDIR(dir->i_mode))	/* actually coding error if occurs*/
		return (ino_t)-1L;
	if (dir->i_ino == MSDOS_ROOT_INO) return dir->i_ino;
	if (!locked) lock_creation(); /* prevent renames */
	if ((current = raw_scan(dir->i_sb,dir->u.msdos_i.i_start,MSDOS_DOTDOT,0L, NULL)) < 0) {
	} else if (!current) this = MSDOS_ROOT_INO;
	else {
		if ((prev = raw_scan(dir->i_sb,current,MSDOS_DOTDOT,0L,NULL)) < 0) {
		} else {
			if (prev == 0 
#ifndef FAT_BITS_32
				&& MSDOS_SB(dir->i_sb)->fat_bits == 32
#endif
		    )
				prev = MSDOS_SB(dir->i_sb)->root_cluster;
			raw_scan(dir->i_sb,prev,NULL,current,&this);
		}
	}
	if (!locked) unlock_creation();
	return this;
}
