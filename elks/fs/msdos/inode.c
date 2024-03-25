/*
 *  linux-0.97/fs/msdos/inode.c
 *
 *  Written 1992 by Werner Almesberger
 */

#include <linuxmt/config.h>
#include <linuxmt/msdos_fs.h>
#include <linuxmt/msdos_fs_i.h>
#include <linuxmt/msdos_fs_sb.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>
#include <linuxmt/devnum.h>
#include <linuxmt/debug.h>

#ifdef CONFIG_FS_DEV
/* FAT device table, increase DEVDIR_SIZE and DEVINO_BASE to add entries*/
struct msdos_devdir_entry devnods[DEVDIR_SIZE] = {
    { "hda",	S_IFBLK | 0644, DEV_HDA                     },
    { "hda1",	S_IFBLK | 0644, DEV_HDA+1                   },
    { "hda2",	S_IFBLK | 0644, DEV_HDA+2                   },
    { "hda3",	S_IFBLK | 0644, DEV_HDA+3                   },
    { "hda4",	S_IFBLK | 0644, DEV_HDA+4                   },
    { "hdb",	S_IFBLK | 0644, DEV_HDB                     },
    { "hdb1",	S_IFBLK | 0644, DEV_HDB+1                   },
    { "hdb2",	S_IFBLK | 0644, DEV_HDB+2                   },
    { "hdb3",	S_IFBLK | 0644, DEV_HDB+3                   },
    { "hdb4",	S_IFBLK | 0644, DEV_HDB+4                   },
    { "hdc",	S_IFBLK | 0644, DEV_HDC                     },
    { "hdc1",	S_IFBLK | 0644, DEV_HDC+1                   },
    { "hdc2",	S_IFBLK | 0644, DEV_HDC+2                   },
    { "hdc3",	S_IFBLK | 0644, DEV_HDC+3                   },
    { "hdc4",	S_IFBLK | 0644, DEV_HDC+4                   },
    { "hdd",	S_IFBLK | 0644, DEV_HDD                     },
    { "hdd1",	S_IFBLK | 0644, DEV_HDD+1                   },
    { "hdd2",	S_IFBLK | 0644, DEV_HDD+2                   },
    { "hdd3",	S_IFBLK | 0644, DEV_HDD+3                   },
    { "hdd4",	S_IFBLK | 0644, DEV_HDD+4                   },
    { "fd0",	S_IFBLK | 0644, DEV_FD0                     },
    { "fd1",	S_IFBLK | 0644, DEV_FD1                     },
    { "fd2",    S_IFBLK | 0644, DEV_FD2                     },
    { "fd3",    S_IFBLK | 0644, DEV_FD3                     },
    { "rd0",	S_IFBLK | 0644, MKDEV(RAM_MAJOR, 0)         },
    { "ssd",	S_IFBLK | 0644, MKDEV(SSD_MAJOR, 0)         },
    { "kmem",	S_IFCHR | 0644, MKDEV(MEM_MAJOR, 2)         },
    { "null",	S_IFCHR | 0644, MKDEV(MEM_MAJOR, 3)         },
    { "zero",	S_IFCHR | 0644, MKDEV(MEM_MAJOR, 5)         },
    { "tty1",	S_IFCHR | 0644, MKDEV(TTY_MAJOR, 0)         },
    { "tty2",	S_IFCHR | 0644, MKDEV(TTY_MAJOR, 1)         },
    { "tty3",	S_IFCHR | 0644, MKDEV(TTY_MAJOR, 2)         },
    { "ttyS0",	S_IFCHR | 0644, MKDEV(TTY_MAJOR, 64)        },
    { "ttyS1",	S_IFCHR | 0644, MKDEV(TTY_MAJOR, 65)        },
    { "console",S_IFCHR | 0600, MKDEV(TTY_MAJOR, 254)       },
    { "tty",	S_IFCHR | 0666, MKDEV(TTY_MAJOR, 255)       },
    { "ttyp0",	S_IFCHR | 0644, MKDEV(TTY_MAJOR, 8)         },
    { "ttyp1",	S_IFCHR | 0644, MKDEV(TTY_MAJOR, 9)         },
    { "ptyp0",	S_IFCHR | 0644, MKDEV(PTY_MASTER_MAJOR, 8)  },
    { "ptyp1",	S_IFCHR | 0644, MKDEV(PTY_MASTER_MAJOR, 9)  },
    { "tcpdev",	S_IFCHR | 0644, MKDEV(TCPDEV_MAJOR, 0)      },
    { "ne0",	S_IFCHR | 0644, MKDEV(ETH_MAJOR, 0)         },
    { "wd0",	S_IFCHR | 0644, MKDEV(ETH_MAJOR, 1)         },
    { "3c0",	S_IFCHR | 0644, MKDEV(ETH_MAJOR, 2)         },
};
#endif

static struct super_operations msdos_sops;

static void msdos_put_inode(register struct inode *inode)
{
	if (inode->i_dirt) debug_fat("put_inode %ld count %d dirty %d\n",
		(unsigned long)inode->i_ino, inode->i_count, inode->i_dirt);
	if (!inode->i_nlink) {
		inode->i_size = 0;
		msdos_truncate(inode);
		clear_inode(inode);
	}
}


static void msdos_put_super(register struct super_block *sb)
{
	debug_fat("put_super\n");
	cache_inval_dev(sb->s_dev);
	lock_super(sb);
	sb->s_dev = 0;
	unlock_super(sb);
	return;
}

static void print_formatted(unsigned long n)
{
	char kbytes_or_mbytes = 'k';

	if (n >= 10000) {
		n /= 1000;
		kbytes_or_mbytes = 'M';
	}
	printk("%lu%c", n, kbytes_or_mbytes);
}

/* Read the super block of an MS-DOS FS. */
static struct super_block *msdos_read_super(struct super_block *s, char *data,
	int silent)
{
	struct msdos_sb_info *sb = MSDOS_SB(s);
	struct msdos_boot_sector *b;
	struct buffer_head *bh;
	unsigned long total_sectors, data_sectors, total_displayed;
	cluster_t max_clusters;
	int fat32;

	cache_init();
	lock_super(s);
	bh = bread(s->s_dev, 0);
	unlock_super(s);
	if (bh == NULL) {
/*		s->s_dev = 0;*/
		printk("FAT: can't read super\n");
		return NULL;
	}

#ifdef CONFIG_VAR_SECTOR_SIZE
        sb->sector_size = get_sector_size(s->s_dev);
	switch (sb->sector_size) {
	case 512:
		sb->sector_bits = 9;	/* log2(sector_size) */
		sb->msdos_dps = 16;		/* SECTOR_SIZE / sizeof(struct msdos_dir_entry) */
		sb->msdos_dps_bits = 4;	/* log2(msdos_dps) */
		break;
	case 1024:
		sb->sector_bits = 10;	/* log2(sector_size) */
		sb->msdos_dps = 32;		/* SECTOR_SIZE / sizeof(struct msdos_dir_entry) */
		sb->msdos_dps_bits = 5;	/* log2(msdos_dps) */
		break;
	default:
		printk("FAT: %d sector size not supported\n", sb->sector_size);
		return NULL;
	}
#endif

	map_buffer(bh);
	b = (struct msdos_boot_sector *) bh->b_data;
	sb->cluster_size = b->cluster_size;
	sb->fats = b->fats;
	sb->fat_start = b->reserved;
	if(!b->fat_length && b->fat32_length){
		fat32 = 1;
		sb->fat_length = (unsigned short)b->fat32_length;
		sb->root_cluster = b->root_cluster;
	} else {
		fat32 = 0;
#ifndef FAT_BITS_32
		sb->fat_length = b->fat_length;
		sb->root_cluster = 0;
#endif
	}
	sb->dir_start= b->reserved + b->fats * sb->fat_length;
	sb->dir_entries = b->dir_entries;
	sb->data_start = sb->dir_start +
		(sb->dir_entries >> (SECTOR_BITS_SB(s) - MSDOS_DIR_BITS));
	total_sectors = b->sectors? b->sectors : b->total_sect;
	data_sectors = total_sectors - sb->data_start;
	sb->clusters = sb->cluster_size?  data_sectors / sb->cluster_size : 0;
	sb->fat_bits = fat32 ? 32 : sb->clusters > MSDOS_FAT12_MAX_CLUSTERS ? 16 : 12;
	sb->previous_cluster = 0;
	unmap_brelse(bh);

printk("FAT: me=%x,csz=%d,#f=%d,floc=%d,fsz=%d,rloc=%d,#d=%d,dloc=%d,#s=%lu,ts=%lu\n",
	b->media, sb->cluster_size, sb->fats, sb->fat_start,
	sb->fat_length, sb->dir_start, sb->dir_entries,
	sb->data_start, total_sectors, b->total_sect);

	/* calculate max clusters based on FAT table size */
	max_clusters = (cluster_t)sb->fat_length *
		(SECTOR_SIZE_SB(s) * 8 / sb->fat_bits) - 2;
	/*
	 * Allow disks created with too small a FAT to be mounted, but limit free space.
	 * (This is a bug in FreeDOS mkfat - MSDOS 6.22 will overwrite root directory
	 * when FAT table expands past limit).
	 * Disk free space will be shown incorrectly between ELKS and MSDOS in this case.
	 */
	if (sb->clusters > max_clusters) {
	    printk("FAT: #clus=%ld > max=%ld, limiting free space\n",
		    sb->clusters, max_clusters);
	    sb->clusters = max_clusters;
	}

	if (!sb->fats || (sb->dir_entries & (MSDOS_DPS_SB(s)-1)) || !b->cluster_size
#ifdef FAT_BITS_32
		|| !fat32
#endif
			) {
		printk("FAT: Unsupported format\n");
		return NULL;
	}

	total_displayed = total_sectors >> (BLOCK_SIZE_BITS - SECTOR_BITS_SB(s));
#if UNUSED      /* calculate free count on mount */
	long free_displayed = 0;
	cluster_t cluster;
	for (cluster = 2; cluster < sb->clusters + 2; cluster++)
		if (!fat_access(s, cluster, -1))
			free_displayed += sb->cluster_size;
	free_displayed = free_displayed >> (BLOCK_SIZE_BITS - SECTOR_BITS_SB(s));
#endif
	printk("FAT: total "); print_formatted(total_displayed);
	//printk(", free ");     print_formatted(free_displayed);
	printk(", fat%d format\n", sb->fat_bits);

	/*s->s_magic = MSDOS_SUPER_MAGIC;*/

	/* set up enough so that it can read an inode */
	s->s_op = &msdos_sops;
	if (!(s->s_mounted = iget(s,(ino_t)MSDOS_ROOT_INO))) {
		printk("FAT: can't read rootdir\n");
		return NULL;
	}

#ifdef CONFIG_FS_DEV
{
	struct msdos_dir_entry *de;
	ino_t ino;
	loff_t pos = 0;
	int i;
	bh = NULL;

	/* if /dev is found in first eight directory entries, turn on devfs filesystem */
	for (i=0; i<8; i++) {
		ino = msdos_get_entry(s->s_mounted, &pos, &bh, &de); 
		if (ino == (ino_t)-1) break;
		if (de->attr == ATTR_DIR && !strncmp(de->name, "DEV        ", 11)) {
			sb->dev_ino = ino;
			break;
		}
	}
	if (bh)
		unmap_brelse(bh);
}
#endif
	return s;
}


static void msdos_statfs(struct super_block *s,struct statfs *sf, int flags)
{
	cluster_t cluster, cluster_size;
	unsigned long total, free;

	cluster_size = MSDOS_SB(s)->cluster_size;
	sf->f_bsize = SECTOR_SIZE_SB(s);
	total  = (MSDOS_SB(s)->clusters * cluster_size) + MSDOS_SB(s)->data_start;
	sf->f_blocks = total >> (BLOCK_SIZE_BITS - SECTOR_BITS_SB(s));
	if (!(flags & UF_NOFREESPACE)) {
		free = 0;
		for (cluster = 2; cluster < MSDOS_SB(s)->clusters + 2; cluster++)
			if (!fat_access(s, cluster, -1))
				free += cluster_size;
		free >>= (BLOCK_SIZE_BITS - SECTOR_BITS_SB(s));
	} else free = -1L;
	sf->f_bfree = free;
	sf->f_bavail = free;
	sf->f_files = 0;
	sf->f_ffree = 0;
}


void msdos_read_inode(register struct inode *inode)
{
	struct buffer_head *bh;
	register struct msdos_dir_entry *raw_entry;
	cluster_t this, nr;
	int fatsz = MSDOS_SB(inode->i_sb)->fat_bits;

	//debug_fat("read_inode %ld\n", (unsigned long)inode->i_ino);
	inode->u.msdos_i.i_busy = 0;
	inode->i_uid = current->uid;
	inode->i_gid = current->gid;
	if (inode->i_ino == MSDOS_ROOT_INO) {
		inode->i_mode = (0777 & ~current->fs.umask) | S_IFDIR;
		inode->i_op = &msdos_dir_inode_operations;
#ifndef FAT_BITS_32
		if (fatsz == 32)
#endif
		{
			nr = inode->u.msdos_i.i_start = MSDOS_SB(inode->i_sb)->root_cluster;
			if (nr) {
				while (nr != -1) {
					inode->i_size += (cluster_t)SECTOR_SIZE(inode) *
						MSDOS_SB(inode->i_sb)->cluster_size;
					if (!(nr = fat_access(inode->i_sb,nr,-1L))) {
						printk("FAT: can't read dir %ld\n", (unsigned long)inode->i_ino);
						break;
					}
				}
			}
#ifndef FAT_BITS_32
		} else {
			inode->u.msdos_i.i_start = 0;
			inode->i_size = MSDOS_SB(inode->i_sb)->dir_entries* sizeof(struct msdos_dir_entry);
#endif
		}
		inode->i_nlink = 1;
		inode->u.msdos_i.i_attrs = 0;
		inode->i_mtime = 0;
		return;
	}
#ifdef CONFIG_FS_DEV
	else if (MSDOS_SB(inode->i_sb)->dev_ino
                 && inode->i_ino < DEVINO_BASE + DEVDIR_SIZE) {
		inode->i_mode = devnods[(int)inode->i_ino - DEVINO_BASE].mode;
		inode->i_uid  = 0;
		inode->i_size = 0;
		inode->i_mtime= current_time();
		inode->i_gid  = 0;
		inode->i_nlink= 1;
		inode->i_rdev = devnods[(int)inode->i_ino - DEVINO_BASE].rdev;
		if (S_ISCHR(inode->i_mode))
			inode->i_op = &chrdev_inode_operations;
		else if (S_ISBLK(inode->i_mode))
			inode->i_op = &blkdev_inode_operations;
		return;
	}
#endif
	if (!(bh = bread32(inode->i_dev, inode->i_ino >> MSDOS_DPB_BITS))) {
	    printk("FAT: read inode fail\n");
		return;
	}
	map_buffer(bh);
	raw_entry = &((struct msdos_dir_entry *)(bh->b_data))[(int)inode->i_ino & (MSDOS_DPB-1)];
	inode->u.msdos_i.i_start = raw_entry->start |
#ifndef FAT_BITS_32
	    ((fatsz == 32)? ((unsigned long)raw_entry->starthi << 16) : 0);
#else
	    ((unsigned long)raw_entry->starthi << 16);
#endif
	if (raw_entry->attr & ATTR_DIR) {
		inode->i_mode = MSDOS_MKMODE(raw_entry->attr,0777 & ~current->fs.umask) | S_IFDIR;
		inode->i_op = &msdos_dir_inode_operations;
		inode->i_nlink = 3;
		inode->i_size = 0;
		/* read FAT chain to set directory size */
		for (this = inode->u.msdos_i.i_start; this && this != -1; this = fat_access(inode->i_sb,this,-1L))
			inode->i_size += (cluster_t)SECTOR_SIZE(inode) *
				MSDOS_SB(inode->i_sb)->cluster_size;
	}
	else {
		inode->i_mode = MSDOS_MKMODE(raw_entry->attr,0755 & ~current->fs.umask) | S_IFREG;
		inode->i_op = &msdos_file_inode_operations_no_bmap;
		inode->i_nlink = 1;
		inode->i_size = raw_entry->size;
	}
	inode->u.msdos_i.i_attrs = raw_entry->attr & ATTR_UNUSED;
	inode->i_mtime = date_dos2unix(raw_entry->time,raw_entry->date);
	unmap_brelse(bh);
}


static void msdos_write_inode(register struct inode *inode)
{
	struct buffer_head *bh;
	register struct msdos_dir_entry *raw_entry;

	inode->i_dirt = 0;
#ifdef CONFIG_FS_DEV
	/* FAT /dev inodes don't actually exist, so don't write anything*/
	if (inode->i_ino < DEVINO_BASE + DEVDIR_SIZE)
		return;
#endif
	debug_fat("write_inode %ld %d\n", (unsigned long)inode->i_ino, inode->i_dirt);
	if (inode->i_ino == MSDOS_ROOT_INO || !inode->i_nlink) return;
	if (!(bh = bread32(inode->i_dev, inode->i_ino >> MSDOS_DPB_BITS))) {
	    printk("FAT: write inode fail\n");
	    return;
	}
	map_buffer(bh);
	raw_entry = &((struct msdos_dir_entry *)(bh->b_data))[(int)inode->i_ino & (MSDOS_DPB-1)];
	if (S_ISDIR(inode->i_mode)) {
		raw_entry->attr = ATTR_DIR;
		raw_entry->size = 0;
	} else {
		raw_entry->attr = ATTR_NONE;
		raw_entry->size = inode->i_size;
	}
	raw_entry->attr |= MSDOS_MKATTR(inode->i_mode) | inode->u.msdos_i.i_attrs;
	raw_entry->start = (unsigned short)inode->u.msdos_i.i_start;
	raw_entry->starthi = (unsigned short)(inode->u.msdos_i.i_start >> 16);
	date_unix2dos(inode->i_mtime,&raw_entry->time,&raw_entry->date);
	debug_fat("write_inode block write %lu\n", buffer_blocknr(bh));
	mark_buffer_dirty(bh);
	unmap_brelse(bh);
}

static struct super_operations msdos_sops = {
	msdos_read_inode,
	msdos_write_inode,
	msdos_put_inode,
	msdos_put_super,
	NULL,		/* write_super*/
	NULL,		/* remount*/
	msdos_statfs
};

struct file_system_type msdos_fs_type = {
	msdos_read_super,
	FST_MSDOS
};

int init_msdos_fs(void)
{
	return 1;
}
