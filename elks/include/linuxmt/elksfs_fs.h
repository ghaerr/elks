#ifndef LX86_LINUXMT_ELKSFS_FS_H
#define LX86_LINUXMT_ELKSFS_FS_H

/*
 * The elksfs filesystem constants/structures
 *
 * Thanks to Kees J Bot for sending me the definitions of the new elksfs
 * filesystem (aka V2) with bigger inodes and 32-bit block pointers.
 */

#define ELKSFS_ROOT_INO 1

/* Not the same as the bogus LINK_MAX in <linux/limits.h>. Oh well. */
#define ELKSFS_LINK_MAX 	250

#define ELKSFS_I_MAP_SLOTS	8
#define ELKSFS_Z_MAP_SLOTS	64
#define MINIX_SUPER_MAGIC	0x137F	/* original elksfs fs */
#define MINIX_SUPER_MAGIC2	0x138F	/* elksfs fs, 30 char names */
#define MINIX2_SUPER_MAGIC	0x2468	/* elksfs V2 fs */
#define MINIX2_SUPER_MAGIC2	0x2478	/* elksfs V2 fs, 30 char names */
#define ELKSFS_SUPER_MAGIC	0x2400	/* elksfs V1 fs */
#define ELKSFS_VALID_FS		0x0001	/* Clean fs. */
#define ELKSFS_ERROR_FS		0x0002	/* fs has errors. */

#define ELKSFS_INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct elksfs_inode)))
#define ELKSFS2_INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct elksfs2_inode)))

#define MINIX_V1		0x0001	/* original minix fs */
#define MINIX_V2		0x0002	/* minix V2 fs */
#define ELKSFS_V1		0x0003	/* elksfs V1 */

/*
 * This is the original elksfs inode layout on disk.
 * Note the 8-bit gid and atime and ctime.
 */
struct elksfs_inode {
    __u16 i_mode;
    __u16 i_uid;
    __u32 i_size;
    __u32 i_time;
    __u8 i_gid;
    __u8 i_nlinks;
    __u16 i_zone[9];
};

/*
 * elksfs super-block data on disk
 */
struct elksfs_super_block {
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

struct elksfs_dir_entry {
    __u16 inode;
    char name[0];
};

#ifdef __KERNEL__

extern int elksfs_rename();
extern int elksfs_bmap();
extern int elksfs_sync_file();

extern int elksfs_lookup(struct inode *,char *,size_t,struct inode **);
extern int elksfs_create(struct inode *,char *,size_t,int,struct inode **);
extern int elksfs_mkdir(struct inode *,char *,size_t,int);
extern int elksfs_rmdir(struct inode *,char *,size_t);
extern int elksfs_unlink(struct inode *,char *,size_t);
extern int elksfs_symlink(struct inode *,char *,size_t,char *);
extern int elksfs_link(struct inode *,struct inode *,char *,size_t);
extern int elksfs_mknod(struct inode *,char *,size_t,int,int);
extern struct inode *elksfs_new_inode(struct inode *);
extern void elksfs_free_inode(struct inode *);
extern unsigned long elksfs_count_free_inodes(struct super_block *);
extern unsigned int elksfs_new_block(struct super_block *);
extern void elksfs_free_block(struct super_block *,unsigned long);
extern unsigned long elksfs_count_free_blocks(struct super_block *);

extern struct buffer_head *elksfs_getblk(struct inode *,unsigned long,int);
extern struct buffer_head *elksfs_bread(struct inode *,block_t,int);

extern void elksfs_truncate(struct inode *);
extern void elksfs_put_super(struct super_block *);
extern struct super_block *elksfs_read_super(struct super_block *,char *,int);
extern int init_elksfs_fs(void);
extern void elksfs_write_super(struct super_block *);
extern int elksfs_remount(struct super_block *,int *,char *);
extern void elksfs_read_inode(struct inode *);
extern void elksfs_write_inode(struct inode *);
extern void elksfs_put_inode(struct inode *);
extern void elksfs_statfs(struct super_block *,struct statfs *,size_t);
extern int elksfs_sync_inode(struct inode *);

extern struct inode_operations elksfs_file_inode_operations;
extern struct inode_operations elksfs_dir_inode_operations;
extern struct inode_operations elksfs_symlink_inode_operations;

#endif

#endif
