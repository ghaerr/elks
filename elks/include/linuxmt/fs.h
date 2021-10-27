#ifndef __LINUXMT_FS_H
#define __LINUXMT_FS_H

/*
 * This file has definitions for some important file table structures etc.
 */

#ifdef __KERNEL__

#include <linuxmt/types.h>
#include <linuxmt/wait.h>
#include <linuxmt/vfs.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/ioctl.h>
#include <linuxmt/net.h>
#include <linuxmt/config.h>

#include <arch/bitops.h>

#ifdef CONFIG_PIPE
#include <linuxmt/pipe_fs_i.h>
#endif

#ifdef CONFIG_MINIX_FS
#include <linuxmt/minix_fs.h>
#include <linuxmt/minix_fs_sb.h>
#endif

#ifdef CONFIG_FS_FAT
#include <linuxmt/msdos_fs_i.h>
#include <linuxmt/msdos_fs_sb.h>
#endif

#ifdef CONFIG_ROMFS_FS
#include <linuxmt/romfs_fs.h>
#endif

#endif /* __KERNEL__ */

#define USE_GETBLK

/*  It's silly to have NR_OPEN bigger than NR_FILE, but I'll fix that later.
 *  Anyway, now the file code is no longer dependent on bitmaps in unsigned
 *  longs, but uses the new fd_set structure..
 */

#define NR_OPEN 	20

#define NR_INODE	96	/* this should be bigger than NR_FILE */
#define NR_FILE 	64	/* this can well be larger on a larger system */
#define NR_SUPER	4

#define BLOCK_SIZE	1024
#define BLOCK_SIZE_BITS 10

#define MAY_EXEC	1
#define MAY_WRITE	2
#define MAY_READ	4

#define FMODE_READ	1
#define FMODE_WRITE	2

#define READ		0
#define WRITE		1

#define SEL_IN		1
#define SEL_OUT		2
#define SEL_EX		4

/*
 *	Passed to namei
 */

#define IS_DIR		1
#define NOT_DIR 	2

/* filesystem types, used by sys_mount*/
#define FST_MINIX	1
#define FST_MSDOS	2
#define FST_ROMFS	3

/*
 * These are the fs-independent mount-flags: up to 16 flags are supported
 */
#define MS_RDONLY	    1	/* mount read-only */
#define MS_NOSUID	    2	/* ignore suid and sgid bits */
#define MS_NODEV	    4	/* disallow access to device special files */
#define MS_NOEXEC	    8	/* disallow program execution */
#define MS_SYNCHRONOUS	   16	/* writes are synced at once */
#define MS_REMOUNT	   32	/* alter flags of a mounted FS */

#define S_APPEND	  256	/* append-only file */
#define S_IMMUTABLE	  512	/* immutable file */

/*
 * Flags that can be altered by MS_REMOUNT
 */
#define MS_RMT_MASK (MS_RDONLY)

#ifdef __KERNEL__

/*
 * Note that read-only etc flags are inode-specific: setting some file-system
 * flags just means all the inodes inherit those flags by default. It might be
 * possible to override it selectively if you really wanted to with some
 * ioctl() that is not currently implemented.
 *
 * Exception: MS_RDONLY is always applied to the entire file system.
 */

#define IS_RDONLY(inode) (((inode)->i_sb) && ((inode)->i_sb->s_flags & MS_RDONLY))
#define IS_NOSUID(inode) ((inode)->i_flags & MS_NOSUID)
#define IS_NODEV(inode) ((inode)->i_flags & MS_NODEV)
#define IS_NOEXEC(inode) ((inode)->i_flags & MS_NOEXEC)
#define IS_SYNC(inode) ((inode)->i_flags & MS_SYNCHRONOUS)

#define IS_APPEND(inode) ((inode)->i_flags & S_APPEND)
#define IS_IMMUTABLE(inode) ((inode)->i_flags & S_IMMUTABLE)

struct buffer_head {
    char			*b_data;	/* Address if in L1 buffer area, else 0 */
    block32_t			b_blocknr;	/* 32-bit block numbers required for FAT */
    kdev_t			b_dev;
    struct buffer_head		*b_next_lru;
    struct buffer_head		*b_prev_lru;
    struct wait_queue		b_wait;
    block_t			b_count;
    seg_t			b_seg;		/* Current (L1 or L2) buffer segment */
    char			b_lock;
    char			b_dirty;
    char			b_uptodate;
    char			b_reserved;
#ifdef CONFIG_FS_EXTERNAL_BUFFER
    seg_t			b_ds;		/* L2 buffer data segment */
    char			*b_L2data;	/* Offset into L2 allocation block */
    char			b_mapcount;	/* count of L2 buffer mapped into L1 */
    unsigned char		b_num;		/* Buffer number, for debugging */
#endif
};

#define BLOCK_READ	0
#define BLOCK_WRITE	1

#define mark_buffer_dirty(bh, st) ((bh)->b_dirty = (st))
#define mark_buffer_clean(bh) ((bh)->b_dirty = 0)
#define buffer_dirty(bh) ((bh)->b_dirty)
#define buffer_clean(bh) (!(bh)->b_dirty)
#define buffer_uptodate(bh) ((bh)->b_uptodate)
#define buffer_locked(bh) ((bh)->b_lock)

#define iget(_a, _b) __iget(_a, _b)

extern void brelse(struct buffer_head *);
extern void bforget(struct buffer_head *);

void wait_on_buffer (struct buffer_head *);
void lock_buffer (struct buffer_head *);
void unlock_buffer (struct buffer_head *);

struct inode {

    /* This stuff is on disk */
    __u16			i_mode;
    __u16			i_uid;
    __u32			i_size;
    __u32			i_mtime;
    __u8			i_gid;
    __u8			i_nlink;

    /* This stuff is just in-memory... */
    ino_t			i_ino;
    kdev_t			i_dev;
    kdev_t			i_rdev;
    time_t			i_atime;
    time_t			i_ctime;
    struct inode_operations	*i_op;
    struct super_block		*i_sb;
    struct inode		*i_next;
    struct inode		*i_prev;
    struct inode		*i_mount;
    struct wait_queue		i_wait;
    unsigned short		i_count;
    unsigned short		i_flags;
    unsigned char		i_lock;
    unsigned char		i_dirt;
    sem_t			i_sem;
#ifdef BLOAT_FS
    unsigned long int		i_blksize;
    unsigned long int		i_blocks;
    unsigned long int		i_version;
    unsigned short int		i_wcount;
    unsigned char int		i_seek;
    unsigned char int		i_update;
#endif

    union {
#ifdef CONFIG_PIPE
		struct pipe_inode_info pipe_i;
#endif
#ifdef CONFIG_MINIX_FS
		struct minix_inode_info minix_i;
#endif
#ifdef CONFIG_FS_FAT
		struct msdos_inode_info msdos_i;
#endif	
#ifdef CONFIG_ROMFS_FS
		struct romfs_inode_info romfs;
#endif
#ifdef CONFIG_SOCKET
		struct socket socket_i;
#endif
		void * generic_i;
    } u;
};

struct file {
    mode_t			f_mode;
    loff_t			f_pos;
    unsigned short		f_flags;
    unsigned short		f_count;
    struct inode		*f_inode;
    struct file_operations	*f_op;
#ifdef BLOAT_FS
    off_t			f_reada;
    unsigned long		f_version;
    void			*private_data;
				/* needed for tty driver, but not ntty */
#endif
};

struct super_block {
    kdev_t			s_dev;
    unsigned char		s_lock;
    unsigned char		s_dirt;
    struct file_system_type	*s_type;
    struct super_operations	*s_op;
    unsigned short int		s_flags;
    struct inode		*s_covered;
    struct inode		*s_mounted;
    struct wait_queue		s_wait;
#ifdef BLOAT_FS
    unsigned char		s_rd_only;
    __u32			s_magic;
    time_t			s_time;
#endif
    union {
#ifdef CONFIG_MINIX_FS
		struct minix_sb_info minix_sb;
#endif
#ifdef CONFIG_FS_FAT
		struct msdos_sb_info msdos_sb;
#endif
#ifdef CONFIG_ROMFS_FS
		struct romfs_super_info romfs;
#endif
		void * generic_sbp;
    } u;
};

void wait_on_super (struct super_block *);
void lock_super (struct super_block *);
void unlock_super (struct super_block *);

/*
 * This is the "filldir" function type, used by readdir() to let
 * the kernel specify what kind of dirent layout it wants to have.
 * This allows the kernel to read directories into kernel space or
 * to have different dirent layouts depending on the binary type.
 */

typedef int (*filldir_t) (char *, char *, size_t, off_t, ino_t);

struct file_operations {
    int				(*lseek) ();
    size_t			(*read)(struct inode *,struct file *,char *,size_t);
    size_t			(*write)(struct inode *,struct file *,char *,size_t);
    int 			(*readdir) ();
    int 			(*select) (struct inode *,struct file *, int flag);
    int 			(*ioctl) ();
    int 			(*open) ();
    void			(*release) (struct inode *, struct file *);
#ifdef BLOAT_FS
    int 			(*fsync) ();
    int 			(*check_media_change) ();
    int 			(*revalidate) ();
#endif
};

struct inode_operations {
    struct file_operations	*default_file_ops;
    int 			(*create) ();
    int 			(*lookup) (struct inode * dir, char * name, size_t len, struct inode ** res);
    int 			(*link) ();
    int 			(*unlink) ();
    int 			(*symlink) ();
    int 			(*mkdir) ();
    int 			(*rmdir) ();
    int 			(*mknod) ();
    int 			(*readlink) (struct inode * i, char * buf, size_t len);
    int 			(*follow_link) ();
#ifdef USE_GETBLK
    struct buffer_head *	(*getblk) (struct inode *, block_t, int);
#endif
    void			(*truncate) ();
#ifdef BLOAT_FS
    int 			(*permission) ();
#endif
};

struct super_operations {
    void			(*read_inode) ();
#ifdef BLOAT_FS
    int 			(*notify_change) ();
#endif
    void			(*write_inode) ();
    void			(*put_inode) ();
    void			(*put_super) ();
    void			(*write_super) ();
#ifdef BLOAT_FS	/* i8086 statfs goes to kernel, then user */
    void			(*statfs_kern) ();
#endif
    int 			(*remount_fs) ();
};

struct file_system_type {
    struct super_block		*(*read_super) ();
    int				type;
};

#ifdef BLOAT_FS
/*
 * Attribute flags.  These should be or-ed together to figure out what
 * has been changed!
 */

#define ATTR_MODE	1
#define ATTR_UID	2
#define ATTR_GID	4
#define ATTR_SIZE	8
#define ATTR_ATIME	16
#define ATTR_MTIME	32
#define ATTR_CTIME	64
#define ATTR_ATIME_SET	128
#define ATTR_MTIME_SET	256
#define ATTR_FORCE	512

/*
 * This is the Inode Attributes structure, used for notify_change(). It uses
 * the above definitions as flags, to know which values have changed. Also in
 * this manner, a Filesystem can look at only the values it cares about.
 *
 * Basically, these are the attributes that the VFS layer can request to
 * change from the FS layer.
 *
 * Derek Atkins <warlord@MIT.EDU> 94-10-20
 */
struct iattr {
    unsigned int		ia_valid;
    umode_t			ia_mode;
    uid_t			ia_uid;
    gid_t			ia_gid;
    off_t			ia_size;
    time_t			ia_atime;
    time_t			ia_mtime;
    time_t			ia_ctime;
};

extern int notify_change(struct inode *,struct iattr *);

extern int event;		/* Event counter */
#endif

extern int sys_open(char *,int,int);
extern int sys_close(unsigned int);	/* yes, it's really unsigned */

/*@-namechecks@*/

extern void _close_allfiles(void);

extern struct inode *__iget(struct super_block *,ino_t);

/*@+namechecks@*/

extern int register_blkdev(unsigned int,char *,struct file_operations *);
extern int unregister_blkdev(void);
extern int blkdev_open(struct inode *,struct file *);

extern struct file_operations def_blk_fops;
extern struct inode_operations blkdev_inode_operations;

extern int register_chrdev(unsigned int,char *,struct file_operations *);
extern int unregister_chrdev(void);
/* extern int chrdev_open(struct inode *,struct file *); */

extern struct file_operations def_chr_fops;
extern struct inode_operations chrdev_inode_operations;

extern void init_fifo(struct inode *);

extern struct file_operations connecting_fifo_fops;
extern struct file_operations read_fifo_fops;
extern struct file_operations write_fifo_fops;
extern struct file_operations rdwr_fifo_fops;
extern struct file_operations read_pipe_fops;
extern struct file_operations write_pipe_fops;
extern struct file_operations rdwr_pipe_fops;
extern struct inode_operations pipe_inode_operations;

#ifdef CONFIG_SOCKET
extern struct inode_operations sock_inode_operations;
#endif

extern int fs_may_mount(kdev_t);
extern int fs_may_umount(kdev_t,struct inode *);
extern int fs_may_remount_ro(kdev_t);

extern struct file file_array[];
extern struct super_block super_blocks[];

extern void invalidate_inodes(kdev_t);
extern void invalidate_buffers(kdev_t);
extern void sync_inodes(kdev_t);
extern void sync_dev(kdev_t);
extern void fsync_dev(kdev_t);
extern void sync_supers(kdev_t);
extern int namei(char *,struct inode **,int,int);

#define lnamei(_a,_b) _namei(_a,NULL,0,_b)

extern int permission(struct inode *,int);

extern int open_namei(char *,int,int,struct inode **,struct inode *);
extern int do_mknod(char *,int,int,dev_t);
extern void iput(struct inode *);

/*extern struct inode *get_empty_inode(void); */
extern struct inode *new_inode(struct inode *dir, __u16 mode);
extern void insert_inode_hash(struct inode *);
extern void clear_inode(struct inode *);
extern int open_filp(unsigned short, struct inode *, struct file **);
extern void close_filp(struct inode *, struct file *);

extern struct buffer_head *get_hash_table(kdev_t,block_t);
extern struct buffer_head *getblk(kdev_t,block_t);
extern struct buffer_head *getblk32(kdev_t,block32_t);
extern struct buffer_head *readbuf(struct buffer_head *);

extern void ll_rw_blk(int,struct buffer_head *);
extern void ll_rw_page(void);

extern void print_bufmap_status(void);

extern void put_super(kdev_t);
extern kdev_t ROOT_DEV;

extern void show_buffers(void);
extern void mount_root(void);

extern int char_read(struct inode *,struct file *,char *,int);

extern int fd_check(unsigned int,char *,size_t,int,struct file **);

#ifdef CONFIG_FS_EXTERNAL_BUFFER
extern void map_buffer(struct buffer_head *);
extern void unmap_buffer(struct buffer_head *);
extern void unmap_brelse(struct buffer_head *);
extern char *buffer_data(struct buffer_head *);
#else
#define map_buffer(bh)
#define unmap_buffer(bh)
#define unmap_brelse(bh) brelse(bh)
#define buffer_data(bh)  ((bh)->b_data)	/* for accessing unmapped buffer data*/
#endif

#ifdef CONFIG_BLK_DEV_CHAR
extern size_t block_read(struct inode *,struct file *,char *,size_t);
extern size_t block_write(struct inode *,struct file *,char *,size_t);
#else
#define block_read NULL
#define block_write NULL
#endif

#ifdef CONFIG_EXEC_COMPRESS
extern size_t decompress(char *buf, seg_t seg, size_t orig_size, size_t compr_size, int safety);
#endif

#ifdef BLOAT_FS
extern int get_write_access(struct inode *);
extern void put_write_access(struct inode *);
#else
#define get_write_access(_a)
#define put_write_access(_a)
#endif

/*@-namechecks@*/

extern int _namei(char *,struct inode *,int,struct inode **);

/*@+namechecks@*/

extern int sys_dup(unsigned int);

extern struct buffer_head *bread(dev_t,block_t);
extern struct buffer_head *bread32(dev_t,block32_t);

extern int open_fd(int flags, struct inode *inode);

extern void mark_buffer_uptodate(struct buffer_head *,int);

#endif /* __KERNEL__ */

#endif
