#ifndef _MSDOS_FS_SB
#define _MSDOS_FS_SB

struct msdos_sb_info { /* space in struct super_block is 28 bytes */
	unsigned short cluster_size; /* sectors/cluster */
	unsigned char fats,fat_bits; /* number of FATs, FAT bits (12 or 16) */
	unsigned short fat_start,fat_length; /* FAT start & length (sec.) */
	unsigned short dir_start,dir_entries; /* root dir start & entries */
	unsigned short data_start;   /* first data sector */
	unsigned long clusters;      /* number of clusters */
	unsigned long root_cluster;
	long previous_cluster;       /* used in add_cluster */
# ifdef CONFIG_FS_DEV
	ino_t dev_ino;               /* "/dev" ino */
# endif
};

# ifdef CONFIG_FS_DEV
#define DEVINO_BASE MSDOS_DPB
#define DEVDIR_SIZE 28

struct msdos_devdir_entry {
    char    *name;
    __u16   mode;
    kdev_t  rdev;
};

extern struct msdos_devdir_entry devnods[DEVDIR_SIZE];

# endif

#endif
