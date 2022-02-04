/*
 *  linux-0.97/fs/msdos/inode.c
 *
 *  Written 1992 by Werner Almesberger
 */

#include <linuxmt/msdos_fs.h>
#include <linuxmt/msdos_fs_i.h>
#include <linuxmt/msdos_fs_sb.h>
#include <linuxmt/bioshd.h>	/* for HDIO_GET_SECTOR_SIZE on bioshd*/
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>
#include <linuxmt/debug.h>

#ifdef CONFIG_FS_DEV
/* FAT device table, increase DEVDIR_SIZE to add entries*/
struct msdos_devdir_entry devnods[DEVDIR_SIZE] = {
    { "hda",	S_IFBLK | 0644, MKDEV(3, 0) },
    { "hda1",	S_IFBLK | 0644, MKDEV(3, 1) },
    { "hda2",	S_IFBLK | 0644, MKDEV(3, 2) },
    { "hda3",	S_IFBLK | 0644, MKDEV(3, 3) },
    { "hda4",	S_IFBLK | 0644, MKDEV(3, 4) },
    { "hdb",	S_IFBLK | 0644, MKDEV(3, 32) },
    { "hdb1",	S_IFBLK | 0644, MKDEV(3, 33) },
    { "hdb2",	S_IFBLK | 0644, MKDEV(3, 34) },
    { "hdb3",	S_IFBLK | 0644, MKDEV(3, 35) },
    { "hdb4",	S_IFBLK | 0644, MKDEV(3, 36) },
#ifdef CONFIG_ARCH_IBMPC
    { "hdc",	S_IFBLK | 0644, MKDEV(3, 64) },
    { "hdd",	S_IFBLK | 0644, MKDEV(3, 96) },
#endif
    { "fd0",	S_IFBLK | 0644, MKDEV(3, 128)},
    { "fd1",	S_IFBLK | 0644, MKDEV(3, 160)},
#ifdef CONFIG_ARCH_PC98
    { "fd2",   S_IFBLK | 0644, MKDEV(3, 192)},
    { "fd3",   S_IFBLK | 0644, MKDEV(3, 224)},
#endif
    { "rd0",	S_IFBLK | 0644, MKDEV(1, 0) },
    { "kmem",	S_IFCHR | 0644, MKDEV(1, 2) },
    { "null",	S_IFCHR | 0644, MKDEV(1, 3) },
    { "zero",	S_IFCHR | 0644, MKDEV(1, 5) },
    { "tcpdev",	S_IFCHR | 0644, MKDEV(8, 0) },
    { "eth",	S_IFCHR | 0644, MKDEV(9, 0) },
    { "tty1",	S_IFCHR | 0644, MKDEV(4, 0) },
    { "tty2",	S_IFCHR | 0644, MKDEV(4, 1) },
    { "ttyp0",	S_IFCHR | 0644, MKDEV(4, 8) },
    { "ttyp1",	S_IFCHR | 0644, MKDEV(4, 9) },
    { "ptyp0",	S_IFCHR | 0644, MKDEV(2, 8) },
    { "ptyp1",	S_IFCHR | 0644, MKDEV(2, 9) },
    { "ttyS0",	S_IFCHR | 0644, MKDEV(4, 64)},
    { "ttyS1",	S_IFCHR | 0644, MKDEV(4, 65)},
    { "console",S_IFCHR | 0600, MKDEV(4, 254)},
    { "tty",	S_IFCHR | 0666, MKDEV(4, 255)}
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


/* Read the super block of an MS-DOS FS. */

static struct super_block *msdos_read_super(struct super_block *s, char *data,
	int silent)
{
	struct msdos_sb_info *sb = MSDOS_SB(s);
	struct msdos_boot_sector *b;
	struct buffer_head *bh;
	long total_sectors, total_displayed, data_sectors;
	char kbytes_or_mbytes = 'k';
	int fat32;

	cache_init();
	lock_super(s);
	bh = bread(s->s_dev, (block_t)0);
	unlock_super(s);
	if (bh == NULL) {
/*		s->s_dev = 0;*/
		printk("FAT: can't read super\n");
		return NULL;
	}

#ifdef CONFIG_VAR_SECTOR_SIZE
	/* get disk sector size using block device ioctl */
	struct file_operations *fops = get_blkfops(MAJOR(s->s_dev));

	if (!fops || !fops->ioctl ||
		(sb->sector_size = fops->ioctl(NULL, NULL, HDIO_GET_SECTOR_SIZE, s->s_dev)) <= 0)
			sb->sector_size = 512;
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
	sb->dir_start= b->reserved + b->fats*sb->fat_length;
	sb->dir_entries = *((unsigned short *) b->dir_entries);
	sb->data_start = sb->dir_start +
		((sb-> dir_entries << MSDOS_DIR_BITS) >> SECTOR_BITS_SB(s));
	total_sectors = *((unsigned short *) b->sectors)?
		*((unsigned short *) b->sectors) : b->total_sect;
	data_sectors = total_sectors - sb->data_start;
	sb->clusters = sb->cluster_size?  data_sectors/sb->cluster_size : 0;
	sb->fat_bits = fat32 ? 32 : sb->clusters > MSDOS_FAT12 ? 16 : 12;
	sb->previous_cluster = 0;
	unmap_brelse(bh);

printk("FAT: me=%x,csz=%d,#f=%d,floc=%d,fsz=%d,rloc=%d,#d=%d,dloc=%d,#s=%ld,ts=%ld\n",
	b->media, sb->cluster_size, sb->fats, sb->fat_start,
	sb->fat_length, sb->dir_start, sb->dir_entries,
	sb->data_start, total_sectors, b->total_sect);

	if (!sb->fats || (sb->dir_entries & (MSDOS_DPS_SB(s)-1))
	    || !b->cluster_size || 
#ifndef FAT_BITS_32
		sb->clusters+2 > (unsigned long) sb->fat_length *
			(SECTOR_SIZE_SB(s) * 8 / sb->fat_bits)
#else
		!fat32
#endif
		) {
/*		s->s_dev = 0;*/
		printk("FAT: Unsupported format\n");
		return NULL;
	}

	total_displayed = total_sectors >> (BLOCK_SIZE_BITS - SECTOR_BITS_SB(s));
	if (total_displayed >= 10000) {
		total_displayed /= 1000;
		kbytes_or_mbytes = 'M';
}
	printk("FAT: %ld%c, fat%d format\n", total_displayed, kbytes_or_mbytes,
		sb->fat_bits);

#ifdef BLOAT_FS
	s->s_magic = MSDOS_SUPER_MAGIC;
#endif
	/* set up enough so that it can read an inode */
	s->s_op = &msdos_sops;
	if (!(s->s_mounted = iget(s,(ino_t)MSDOS_ROOT_INO))) {
/*		s->s_dev = 0;*/
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
		if (ino == -1) break;
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


#ifdef BLOAT_FS
static void msdos_statfs(struct super_block *s,struct statfs *buf)
{
	cluster_t cluster_size,free,this;
	struct statfs tmp;

	cluster_size = MSDOS_SB(s)->cluster_size;
	tmp.f_type = s->s_magic;
	tmp.f_bsize = SECTOR_SIZE_SB(s);
	tmp.f_blocks = MSDOS_SB(s)->clusters * cluster_size;
	free = 0;
	for (this = 2; this < MSDOS_SB(s)->clusters+2; this++)
		if (!fat_access(s,this,-1L))
			free++;
	free *= cluster_size;
	tmp.f_bfree = free;
	tmp.f_bavail = free;
	tmp.f_files = 0;
	tmp.f_ffree = 0;
	memcpy(buf, &tmp, bufsiz);
}
#endif


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
	else if (inode->i_ino < DEVINO_BASE + DEVDIR_SIZE) {
		inode->i_mode = devnods[(int)inode->i_ino - DEVINO_BASE].mode;
		inode->i_uid  = 0;
		inode->i_size = 0;
		inode->i_mtime= CURRENT_TIME;
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
	if (!(bh = bread(inode->i_dev,(block_t)(inode->i_ino >> MSDOS_DPB_BITS)))) {
	    printk("FAT: read inode fail\n");
		return;
	}
	map_buffer(bh);
	raw_entry = &((struct msdos_dir_entry *)(bh->b_data))[(int)inode->i_ino & (MSDOS_DPB-1)];
	((unsigned short *)&inode->u.msdos_i.i_start)[0] = raw_entry->start;
	((unsigned short *)&inode->u.msdos_i.i_start)[1] = 
#ifndef FAT_BITS_32
	    (fatsz == 32)? raw_entry->starthi : 0;
#else
	    raw_entry->starthi;
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
	if (!(bh = bread(inode->i_dev,(block_t)(inode->i_ino >> MSDOS_DPB_BITS)))) {
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
	raw_entry->starthi = ((unsigned short *)&inode->u.msdos_i.i_start)[1];
	date_unix2dos(inode->i_mtime,&raw_entry->time,&raw_entry->date);
	debug_fat("write_inode block write %lu\n", bh->b_blocknr);
	bh->b_dirty = 1;
	unmap_brelse(bh);
}

static struct super_operations msdos_sops = {
	msdos_read_inode,
	msdos_write_inode,
	msdos_put_inode,
	msdos_put_super,
	NULL, /* write_super*/
#ifdef BLOAT_FS
	msdos_statfs,
#endif
	NULL
};

struct file_system_type msdos_fs_type = {
	msdos_read_super,
	FST_MSDOS
};

int init_msdos_fs(void)
{
	return 1;
}
