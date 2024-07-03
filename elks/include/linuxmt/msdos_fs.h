#ifndef _MSDOS_FS_H
#define _MSDOS_FS_H

/*
 * The MS-DOS filesystem constants/structures
 */

#include <linuxmt/fs.h>
#include <linuxmt/ctype.h>

#if defined(CONFIG_FARTEXT_KERNEL) && !defined(__STRICT_ANSI__)
#define FATPROC __far __attribute__ ((far_section, noinline, section (".fartext.fatfs")))
#else
#define FATPROC
#endif

#define MSDOS_ROOT_INO  1		/* == MINIX_ROOT_INO */

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

#define MSDOS_FAT12_MAX_CLUSTERS    4084 /* maximum number of clusters in a 12 bit FAT */

struct msdos_boot_sector {
	char ignored[13];	    /*0*/
	unsigned char cluster_size; /* sectors/cluster 13*/
	unsigned short reserved;    /* reserved sectors 14*/
	unsigned char fats;	    /* number of FATs 16*/
	unsigned short dir_entries; /* root directory entries 17*/
	unsigned short sectors;     /* number of sectors 19*/
	unsigned char media;	    /* media code (unused) 21*/
	unsigned short fat_length;  /* sectors/FAT 22*/
	unsigned short secs_track;  /* sectors per track (unused) 24*/
	unsigned short heads;	    /* number of heads (unused) 26*/
	unsigned long hidden;	    /* hidden sectors 28*/
	unsigned long total_sect;   /* number of sectors (if sectors == 0) 32*/

	/* The following fields are only used by FAT32 */
	__u32	fat32_length;	/* sectors/FAT 36 */
	__u16	flags;		/* bit 8: fat mirroring, low 4: active fat (unused) */
	__u8	version[2];	/* major, minor filesystem version (unused) */
	__u32	root_cluster;	/* first cluster of root directory 44 */
	__u16	info_sector;	/* filesystem info sector (unused) */
	__u16	backup_boot;	/* backup boot sector (unused) */
	__u16	reserved2[6];	/* Unused */
} __attribute__((packed));

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
	kdev_t device; /* device number. 0 means unused. */
	ino_t ino; /* inode number. */
	cluster_t file_cluster; /* cluster number in the file. */
	cluster_t disk_cluster; /* cluster number on disk. */
	struct fat_cache *next; /* next cache entry */
};

/* Convert attribute bits and a mask to the UNIX mode. */

#define MSDOS_MKMODE(a,m) (m & (a & ATTR_RO ? 0444 : 0777))

/* Convert the UNIX mode to MS-DOS attribute bits. */

#define MSDOS_MKATTR(m) (!(m & 0200) ? ATTR_RO : ATTR_NONE)

/* misc.c */

struct buffer_head * FATPROC msdos_sread_nomap(struct super_block *s, sector_t sector, size_t *offset);
struct buffer_head * FATPROC msdos_sread(struct super_block *s, sector_t sector, void **start);
void FATPROC lock_creation(void);
void FATPROC unlock_creation(void);
int  FATPROC msdos_add_cluster(struct inode *inode);
long FATPROC date_dos2unix(unsigned short time,unsigned short date);
void FATPROC date_unix2dos(long unix_date,unsigned short *time, unsigned short *date);
ino_t FATPROC msdos_get_entry(struct inode *dir,loff_t *pos,struct buffer_head **bh,
    struct msdos_dir_entry **de);
int  FATPROC msdos_scan(struct inode *dir,char *name,struct buffer_head **res_bh,
    struct msdos_dir_entry **res_de,ino_t *ino);
ino_t FATPROC msdos_parent_ino(struct inode *dir,int locked);

/* fat.c */

cluster_t FATPROC fat_access(struct super_block *sb,cluster_t this,cluster_t new_value);
sector_t FATPROC msdos_smap(struct inode *inode, sector_t sector);
int  FATPROC fat_free(struct inode *inode,long skip);
void FATPROC cache_init(void);
void FATPROC cache_lookup(struct inode *inode,cluster_t cluster,
	cluster_t *f_clu, cluster_t *d_clu);
void FATPROC cache_add(struct inode *inode, cluster_t f_clu, cluster_t d_clu);
void FATPROC cache_inval_inode(struct inode *inode);
void FATPROC cache_inval_dev(kdev_t device);
cluster_t FATPROC get_cluster(struct inode *inode, cluster_t cluster);

/* namei.c */

extern int msdos_lookup(struct inode *dir,const char *name,size_t len,
	struct inode **result);
extern int msdos_create(struct inode *dir,const char *name,size_t len,mode_t mode,
	struct inode **result);
extern int msdos_mkdir(struct inode *dir,const char *name,size_t len,mode_t mode);
extern int msdos_rmdir(struct inode *dir,const char *name,int len);
extern int msdos_unlink(struct inode *dir,const char *name,int len);
extern int msdos_rename(struct inode *old_dir,const char *old_name,int old_len,
	struct inode *new_dir,const char *new_name,int new_len);

/* inode.c */

extern void msdos_read_inode(struct inode *inode);
extern int init_msdos_fs(void);

/* dir.c */

extern struct inode_operations msdos_dir_inode_operations;
int FATPROC msdos_get_entry_long(struct inode *dir, off_t *pos, struct buffer_head **bh,
    char *name, int *namelen, off_t *dirpos, ino_t *ino);

/* file.c */

extern struct inode_operations msdos_file_inode_operations;
extern struct inode_operations msdos_file_inode_operations_no_bmap;
extern void msdos_truncate(struct inode *inode);

#endif
