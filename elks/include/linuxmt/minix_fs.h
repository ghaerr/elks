#ifndef __LINUXMT_MINIX_FS_H
#define __LINUXMT_MINIX_FS_H

/* The minix filesystem constants/structures
 *
 * Thanks to Kees J Bot for sending me the definitions of the new minix
 * filesystem (aka V2) with bigger inodes and 32-bit block pointers.
 */

#include <linuxmt/types.h>

#define MINIX_ROOT_INO 1

#define MINIX_SUPER_BLOCK   1

#define MINIX_LINK_MAX	250

/* MINIX V1 buffers for inode and zone bitmaps*/
#define MINIX_I_MAP_SLOTS	4
#define MINIX_Z_MAP_SLOTS	8

#define MINIX_SUPER_MAGIC	0x137F	/* original minix fs */
#define MINIX_SUPER_MAGIC2	0x138F	/* minix fs, 30 char names */
#define MINIX2_SUPER_MAGIC	0x2468	/* minix V2 fs */
#define MINIX2_SUPER_MAGIC2	0x2478	/* minix V2 fs, 30 char names */
#define MINIX_VALID_FS		0x0001	/* Clean fs. */
#define MINIX_ERROR_FS		0x0002	/* fs has errors. */

#define MINIX_INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct minix_inode)))
#define MINIX2_INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct minix2_inode)))

struct super_block;
struct inode;

/* inode in-kernel data */

struct minix_inode_info {
    __u16	i_zone[9];
};

/*  This is the original minix inode layout on disk.
 *  Note the 8-bit gid and atime and ctime.
 */
struct minix_inode {
    __u16	i_mode;
    __u16	i_uid;
    __u32	i_size;
    __u32	i_time;
    __u8	i_gid;
    __u8	i_nlinks;
    __u16	i_zone[9];
};

/*
 * minix super-block data on disk
 */
struct minix_super_block {
    __u16	s_ninodes;
    __u16	s_nzones;
    __u16	s_imap_blocks;
    __u16	s_zmap_blocks;
    __u16	s_firstdatazone;
    __u16	s_log_zone_size;
    __u32	s_max_size;
    __u16	s_magic;
    __u16	s_state;
    __u32	s_zones;		/* MINIX V2 only*/
};

struct minix_dir_entry {
    __u16	inode;
    char	name[];
};

#ifdef __KERNEL__

extern unsigned short minix_bmap(register struct inode *,block_t,int);
extern struct buffer_head *minix_bread(struct inode *,block_t,int);
extern struct buffer_head *get_map_block(kdev_t dev, block_t block);
extern unsigned short minix_count_free_blocks(register struct super_block *);
extern unsigned short minix_count_free_inodes(register struct super_block *);
extern int minix_create(register struct inode *,const char *,size_t,mode_t,
			struct inode **);
extern void minix_free_block(register struct super_block *,block_t);
extern void minix_free_inode(register struct inode *);
extern struct buffer_head *minix_getblk(register struct inode *,block_t,int);
extern int minix_link(register struct inode *,char *,size_t,
			register struct inode *);
extern int minix_lookup(register struct inode *,const char *,size_t,
			register struct inode **);
extern int minix_mkdir(register struct inode *,const char *,size_t,mode_t);
extern int minix_mknod(register struct inode *,const char *,size_t,mode_t,int);
extern block_t minix_new_block(register struct super_block *);
extern struct inode *minix_new_inode(struct inode *,mode_t);
/*extern void minix_put_inode(register struct inode *);*/
extern void minix_put_super(register struct super_block *);
/*extern void minix_read_inode(register struct inode *);*/
extern struct super_block *minix_read_super(register struct super_block *,
					    char *,int);
extern int minix_remount(register struct super_block *,int *,char *);
extern int minix_rmdir(register struct inode *,char *,size_t);
extern void minix_set_ops(struct inode *);
extern void minix_statfs(struct super_block *,struct statfs *, int);
extern int minix_symlink(struct inode *,char *,size_t,char *);
extern int minix_sync_inode(register struct inode *);
extern void minix_truncate(register struct inode *);
extern int minix_unlink(struct inode *,char *,size_t);
extern void minix_write_inode(register struct inode *);
extern void minix_write_super(register struct super_block *);

extern int init_minix_fs(void);

extern struct inode_operations minix_file_inode_operations;
extern struct inode_operations minix_dir_inode_operations;
extern struct inode_operations minix_symlink_inode_operations;

#endif
#endif
