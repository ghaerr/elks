#ifndef _MSDOS_FS_H
#define _MSDOS_FS_H

/*
 * The MS-DOS filesystem constants/structures
 */

#include <linuxmt/fs.h>
#include <linuxmt/ctype.h>

#ifndef toupper
extern char toupper(char c);
#endif
#ifndef tolower
extern char tolower(char c);
#endif

#define MSDOS_ROOT_INO  1 /* == MINIX_ROOT_INO */
#define SECTOR_SIZE     512 /* sector size (bytes) */
#define SECTOR_BITS	9 /* log2(SECTOR_SIZE) */
#define MSDOS_DPB	(MSDOS_DPS*2) /* dir entries per block */
#define MSDOS_DPB_BITS	5 /* log2(MSDOS_DPB) */
#define MSDOS_DPS	(SECTOR_SIZE/sizeof(struct msdos_dir_entry))
#define MSDOS_DPS_BITS	4 /* log2(MSDOS_DPS) */
#define MSDOS_DIR_BITS	5 /* log2(sizeof(struct msdos_dir_entry)) */

#define MSDOS_SUPER_MAGIC 0x4d44 /* MD */

#define FAT_CACHE    8 /* FAT cache size */

#define ATTR_RO      1  /* read-only */
#define ATTR_HIDDEN  2  /* hidden */
#define ATTR_SYS     4  /* system */
#define ATTR_VOLUME  8  /* volume label */
#define ATTR_DIR     16 /* directory */
#define ATTR_ARCH    32 /* archived */

#define ATTR_NONE    0 /* no attribute bits */
#define ATTR_UNUSED  (ATTR_VOLUME | ATTR_ARCH | ATTR_SYS | ATTR_HIDDEN)
	/* attribute bits that are copied "as is" */
#define ATTR_EXT     (ATTR_RO | ATTR_HIDDEN | ATTR_SYS | ATTR_VOLUME)
#define DELETED_FLAG 0xe5 /* marks file as deleted when in name[0] */
#define IS_FREE(n) (!*(n) || *(unsigned char *) (n) == DELETED_FLAG)

#define MSDOS_SB(s) (&((s)->u.msdos_sb))
#define MSDOS_I(i) (&((i)->u.msdos_i))

#define MSDOS_NAME 11 /* maximum name length */
#define MSDOS_DOT    ".          " /* ".", padded to MSDOS_NAME chars */
#define MSDOS_DOTDOT "..         " /* "..", padded to MSDOS_NAME chars */

#define MSDOS_FAT12 4078 /* maximum number of clusters in a 12 bit FAT */

struct msdos_boot_sector {
	char ignored[13];		/*0*/
	unsigned char cluster_size; /* sectors/cluster 13*/
	unsigned short reserved;    /* reserved sectors 14*/
	unsigned char fats;	    /* number of FATs 16*/
	unsigned char dir_entries[2];/* root directory entries 17*/
	unsigned char sectors[2];   /* number of sectors 19*/
	unsigned char media;	    /* media code (unused) 21*/
	unsigned short fat_length;  /* sectors/FAT 22*/
	unsigned short secs_track;  /* sectors per track (unused) 24*/
	unsigned short heads;	    /* number of heads (unused) 26*/
	unsigned long hidden;	    /* hidden sectors (unused) 28*/
	unsigned long total_sect;   /* number of sectors (if sectors == 0) 32*/

	/* The following fields are only used by FAT32 */
	__u32	fat32_length;	/* sectors/FAT */
	__u16	flags;		/* bit 8: fat mirroring, low 4: active fat */
	__u8	version[2];	/* major, minor filesystem version */
	__u32	root_cluster;	/* first cluster in root directory */
	__u16	info_sector;	/* filesystem info sector */
	__u16	backup_boot;	/* backup boot sector */
	__u16	reserved2[6];	/* Unused */
};

struct msdos_dir_entry {
	char name[8],ext[3]; /* name and extension */
	unsigned char attr;  /* attribute bits */
	char unused[8];
	unsigned short starthi;
	unsigned short time,date,start; /* time, date and first cluster */
	unsigned long size;  /* file size (in bytes) */
};

#define LAST_LONG_ENTRY		0x40	/* marks last entry in slot seqno*/

/* Up to 13 characters of the name */
struct msdos_dir_slot {
	__u8    id;		/* sequence number for slot */
	__u8    name0_4[10];	/* first 5 characters in name */
	__u8    attr;		/* attribute byte */
	__u8    reserved;	/* always 0 */
	__u8    alias_checksum;	/* checksum for 8.3 alias */
	__u8    name5_10[12];	/* 6 more characters in name */
	__u16   start;		/* starting cluster number, 0 in long slots */
	__u8    name11_12[4];	/* last 2 characters in name */
};

struct slot_info {
	int is_long;		       /* was the found entry long */
	int long_slots;		       /* number of long slots in filename */
	//int total_slots;	       /* total slots (long and short) */
	loff_t longname_offset;	       /* dir offset for longname start */
	loff_t shortname_offset;       /* dir offset for shortname start */
	ino_t ino;		       /* ino for the file */
};

struct fat_cache {
	int device; /* device number. 0 means unused. */
	ino_t ino; /* inode number. */
	long file_cluster; /* cluster number in the file. */
	long disk_cluster; /* cluster number on disk. */
	struct fat_cache *next; /* next cache entry */
};

/* Convert attribute bits and a mask to the UNIX mode. */

#define MSDOS_MKMODE(a,m) (m & (a & ATTR_RO ? 0444 : 0777))

/* Convert the UNIX mode to MS-DOS attribute bits. */

#define MSDOS_MKATTR(m) (!(m & 0200) ? ATTR_RO : ATTR_NONE)

extern struct buffer_head *msdos_sread(int dev,long sector,void **start);

/* misc.c */

extern void lock_creation(void);
extern void unlock_creation(void);
extern int msdos_add_cluster(struct inode *inode);
extern long date_dos2unix(unsigned short time,unsigned short date);
extern void date_unix2dos(long unix_date,unsigned short *time,
    unsigned short *date);
extern ino_t msdos_get_entry(struct inode *dir,loff_t *pos,struct buffer_head **bh,
    struct msdos_dir_entry **de);
extern int msdos_scan(struct inode *dir,char *name,struct buffer_head **res_bh,
    struct msdos_dir_entry **res_de,ino_t *ino);
extern ino_t msdos_parent_ino(struct inode *dir,int locked);

/* fat.c */

extern long fat_access(struct super_block *sb,long this,long new_value);
extern long msdos_smap(struct inode *inode,long sector);
extern int fat_free(struct inode *inode,long skip);
extern void cache_init(void);
void cache_lookup(struct inode *inode,long cluster,long *f_clu,long *d_clu);
void cache_add(struct inode *inode,long f_clu,long d_clu);
void cache_inval_inode(struct inode *inode);
void cache_inval_dev(int device);
long get_cluster(struct inode *inode,long cluster);

/* namei.c */

extern int msdos_lookup(struct inode *dir,const char *name,int len,
	struct inode **result);
extern int msdos_create(struct inode *dir,const char *name,int len,int mode,
	struct inode **result);
extern int msdos_mkdir(struct inode *dir,const char *name,int len,int mode);
extern int msdos_rmdir(struct inode *dir,const char *name,int len);
extern int msdos_unlink(struct inode *dir,const char *name,int len);
extern int msdos_rename(struct inode *old_dir,const char *old_name,int old_len,
	struct inode *new_dir,const char *new_name,int new_len);

/* inode.c */

extern void msdos_put_inode(struct inode *inode);
extern void msdos_put_super(struct super_block *sb);
extern struct super_block *msdos_read_super(register struct super_block *s,char *data, int silent);
extern void msdos_statfs(struct super_block *sb,struct statfs *buf);
extern long msdos_bmap(struct inode *inode,long block);
extern void msdos_read_inode(struct inode *inode);
extern void msdos_write_inode(struct inode *inode);
extern int init_msdos_fs(void);

/* dir.c */

//extern struct file_operations msdos_dir_operations;
extern struct inode_operations msdos_dir_inode_operations;
extern int msdos_get_entry_long(struct inode *dir, off_t *pos, struct buffer_head **bh,
    char *name, int *namelen, off_t *dirpos, ino_t *ino);

/* file.c */

//extern struct file_operations msdos_file_operations;
extern struct inode_operations msdos_file_inode_operations;
extern struct inode_operations msdos_file_inode_operations_no_bmap;

extern void msdos_truncate(struct inode *inode);

#endif
