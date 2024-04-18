#ifndef _MSDOS_FS_SB
#define _MSDOS_FS_SB

struct msdos_sb_info { /* space in struct super_block is 28 bytes */
	unsigned short cluster_size; /* sectors/cluster */
	unsigned char fats;          /* number of FATs */
	unsigned char fat_bits;      /* FAT bits (12 or 16) */
	unsigned short fat_start;    /* FAT start sector */
	unsigned short fat_length;   /* FAT length in sectors */
	unsigned short dir_start;    /* root dir start cluster */
	unsigned short dir_entries;  /* root dir # entries */
	unsigned short data_start;   /* first data sector */
	unsigned long clusters;      /* number of clusters */
	unsigned long root_cluster;  /* root directory cluster */
	long previous_cluster;       /* used in add_cluster */
	ino_t dev_ino;               /* "/dev" ino */
#ifdef CONFIG_VAR_SECTOR_SIZE
	int sector_size;             /* physical sector size */
	unsigned char sector_bits;   /* log2(sector_size) */
	unsigned char msdos_dps;     /* dir entries / sector */
	unsigned char msdos_dps_bits;/* log2(msdos_dbs) */
#endif
};

#define MSDOS_DIR_BITS	5		/* log2(sizeof(struct msdos_dir_entry)) */
#define MSDOS_DPB	(BLOCK_SIZE/sizeof(struct msdos_dir_entry)) /* dir entries/block */
#define MSDOS_DPB_BITS	5		/* log2(MSDOS_DPB) */

#ifdef CONFIG_VAR_SECTOR_SIZE
/* variable sector size across disks - use calculated bits in block device superblock*/
#define SECTOR_SIZE(inode)		(MSDOS_SB(inode->i_sb)->sector_size)
#define SECTOR_BITS(inode)		(MSDOS_SB(inode->i_sb)->sector_bits)
#define SECTOR_SIZE_SB(sb)		(MSDOS_SB(sb)->sector_size)
#define SECTOR_BITS_SB(sb)		(MSDOS_SB(sb)->sector_bits)
#define MSDOS_DPS_SB(sb)		(MSDOS_SB(sb)->msdos_dps)
#define MSDOS_DPS_BITS(inode)	(MSDOS_SB(inode->i_sb)->msdos_dps_bits)
#else
/* fixed sector size - uses constants for code size */
#define SECTOR_SIZE(inode)		512		/* sector size (bytes) */
#define SECTOR_BITS(inode)		9		/* log2(SECTOR_SIZE) */
#define SECTOR_SIZE_SB(sb)		512		/* sector size (bytes) */
#define SECTOR_BITS_SB(sb)		9		/* log2(SECTOR_SIZE) */
#define MSDOS_DPS_SB(sb)		(512/sizeof(struct msdos_dir_entry)) /* dirents/sector */
#define MSDOS_DPS_BITS(inode)	4		/* log2(MSDOS_DPS) */
#endif

#ifdef CONFIG_FS_DEV
#define DEVDIR_SIZE             44              /* # entries in FAT device table */
#define DEVINO_BASE             (MSDOS_DPB*2)   /* (DEVDIR_SIZE+MSDOS_DBP-1) & ~(MSDOS_DBP-1) */

struct msdos_devdir_entry {
    const char *name;
    __u16   mode;
    kdev_t  rdev;
};

extern struct msdos_devdir_entry devnods[DEVDIR_SIZE];
#endif

#endif
