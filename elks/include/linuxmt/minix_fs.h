#ifndef LX86_LINUXMT_MINIX_FS_H
#define LX86_LINUXMT_MINIX_FS_H

/* The minix filesystem constants/structures
 *
 * Thanks to Kees J Bot for sending me the definitions of the new minix
 * filesystem (aka V2) with bigger inodes and 32-bit block pointers.
 */

#define MINIX_ROOT_INO 1

/* Not the same as the bogus LINK_MAX in <linux/limits.h>. Oh well. */
#define MINIX_LINK_MAX	250

#define MINIX_I_MAP_SLOTS	8
#define MINIX_Z_MAP_SLOTS	64
#define MINIX_SUPER_MAGIC	0x137F	/* original minix fs */
#define MINIX_SUPER_MAGIC2	0x138F	/* minix fs, 30 char names */
#define MINIX2_SUPER_MAGIC	0x2468	/* minix V2 fs */
#define MINIX2_SUPER_MAGIC2	0x2478	/* minix V2 fs, 30 char names */
#define MINIX_VALID_FS		0x0001	/* Clean fs. */
#define MINIX_ERROR_FS		0x0002	/* fs has errors. */

#define MINIX_INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct minix_inode)))
#define MINIX2_INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct minix2_inode)))

#define MINIX_V1		0x0001	/* original minix fs */
#define MINIX_V2		0x0002	/* minix V2 fs */

#define INODE_VERSION(inode)	inode->i_sb->u.minix_sb.s_version

/*
 * This is the original minix inode layout on disk.
 * Note the 8-bit gid and atime and ctime.
 */
struct minix_inode {
    __u16 i_mode;
    __u16 i_uid;
    __u32 i_size;
    __u32 i_time;
    __u8 i_gid;
    __u8 i_nlinks;
    __u16 i_zone[9];
};

/*
 * minix super-block data on disk
 */
struct minix_super_block {
    __u16 s_ninodes;
    __u16 s_nzones;
    __u16 s_imap_blocks;
    __u16 s_zmap_blocks;
    __u16 s_firstdatazone;
    __u16 s_log_zone_size;
    __u32 s_max_size;
    __u16 s_magic;
    __u16 s_state;
    __u32 s_zones;
};

struct minix_dir_entry {
    __u16 inode;
    char name[0];
};

#ifdef __KERNEL__

extern int minix_lookup(struct inode *,char *,int,struct inode **);
extern int minix_create(struct inode *,char *,int,int,struct inode **);
extern int minix_mkdir(struct inode *,char *,int,int);
extern int minix_rmdir(struct inode *,char *,int);
extern int minix_unlink(struct inode *,char *,int);
extern int minix_symlink(struct inode *,char *,int,char *);
extern int minix_link(struct inode *,struct inode *,char *,int);
extern int minix_mknod(struct inode *,char *,int,int,int);
extern int minix_rename(void);
extern struct inode *minix_new_inode(struct inode *);
extern void minix_free_inode(struct inode *);
extern unsigned long minix_count_free_inodes(struct super_block *);
extern unsigned int minix_new_block(struct super_block *);
extern void minix_free_block(struct super_block *,unsigned short);
extern unsigned long minix_count_free_blocks(struct super_block *);
extern int minix_bmap(void);

extern struct buffer_head *minix_getblk(struct inode *,unsigned short,int);
extern struct buffer_head *minix_bread(struct inode *,unsigned short,int);

extern void minix_truncate(struct inode *);
extern void minix_put_super(struct super_block *);
extern struct super_block *minix_read_super(struct super_block *,char *,int);
extern int init_minix_fs(void);
extern void minix_write_super(struct super_block *);
extern int minix_remount(struct super_block *, int *,char *);
extern void minix_read_inode(struct inode *);
extern void minix_write_inode(struct inode *);
extern void minix_put_inode(struct inode *);
extern void minix_statfs(struct super_block *,struct statfs *,int);
extern int minix_sync_inode(struct inode *);
extern int minix_sync_file(void);

extern struct inode_operations minix_file_inode_operations;
extern struct inode_operations minix_dir_inode_operations;
extern struct inode_operations minix_symlink_inode_operations;

#endif

#endif
