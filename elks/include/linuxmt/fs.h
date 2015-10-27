#ifndef LX86_LINUXMT_FS_H
#define LX86_LINUXMT_FS_H

/*
 * This file has definitions for some important file table structures etc.
 */

#include <linuxmt/types.h>
#include <linuxmt/wait.h>
#include <linuxmt/vfs.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/ioctl.h>
#include <linuxmt/pipe_fs_i.h>
#include <linuxmt/net.h>
#include <linuxmt/config.h>

#include <arch/bitops.h>

/*  It's silly to have NR_OPEN bigger than NR_FILE, but I'll fix that later.
 *  Anyway, now the file code is no longer dependent on bitmaps in unsigned
 *  longs, but uses the new fd_set structure..
 */

#ifdef CONFIG_SHORT_FILES
#define NR_OPEN 	16
#else
#define NR_OPEN 	20
#endif

#define NR_INODE	96	/* this should be bigger than NR_FILE */
#define NR_FILE 	64	/* this can well be larger on a larger system */
#define NR_SUPER	4
#define NR_EXT_BUFFERS	64	/* This may be assumed by some code! */
#define NR_MAPBUFS	8	/* Maximum number of mappable buffers */
#define BLOCK_SIZE	1024
#define BLOCK_SIZE_BITS 10

#ifdef CONFIG_FS_EXTERNAL_BUFFER
#define NR_BUFFERS NR_EXT_BUFFERS
#else
#define NR_BUFFERS NR_MAPBUFS
#endif

#define MAY_EXEC	1
#define MAY_WRITE	2
#define MAY_READ	4

#define FMODE_READ	1
#define FMODE_WRITE	2

#define READ		0
#define WRITE		1
#define READA		2	/* read-ahead - don't pause */
#define WRITEA		3	/* "write-ahead" - silly, but somewhat useful */

#define SEL_IN		1
#define SEL_OUT		2
#define SEL_EX		4

/*
 *	Passed to namei
 */

#define IS_DIR		1
#define NOT_DIR 	2

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

/*
 * Executable formats
 */

#define RUNNABLE_MINIX	0x1
#define RUNNABLE_MSDOS	0x2

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

#ifdef __KERNEL__

extern void buffer_init(void);
extern void inode_init(void);

#ifndef CONFIG_FS_EXTERNAL_BUFFER

#define map_buffer(_a)
#define unmap_buffer(_a)
#define unmap_brelse(_a) brelse(_a)

#endif

struct buffer_head {
    char			*b_data;	/* Address in L1 buffer area */
    block_t			b_blocknr;
    kdev_t			b_dev;
    struct buffer_head		*b_next;
    struct buffer_head		*b_next_lru;
    block_t			b_count;
    struct buffer_head		*b_prev_lru;
    struct wait_queue		b_wait;
    char			b_uptodate;
    char			b_dirty;
    char			b_lock;
    seg_t			b_seg;

#ifdef CONFIG_FS_EXTERNAL_BUFFER
    unsigned char		b_num;		/* Used to lookup L2 area */
    unsigned int		b_mapcount;	/* Used for the new L2 buffer cache scheme */
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

#include <linuxmt/romfs_fs_i.h>

struct inode {

    /* This stuff is on disk */
    __u16			i_mode;
    __u16			i_uid;
    __u32			i_size;
    __u32			i_mtime;
    __u8			i_gid;
    __u8			i_nlink;
    __u16			i_zone[9];

    /* This stuff is just in-memory... */
    ino_t			i_ino;
    kdev_t			i_dev;
    kdev_t			i_rdev;
    time_t			i_atime;
    time_t			i_ctime;

#ifdef BLOAT_FS
    unsigned long int		i_blksize;
    unsigned long int		i_blocks;
    unsigned long int		i_version;
    unsigned short int		i_wcount;
    unsigned char int		i_seek;
    unsigned char int		i_update;
#endif

    struct inode_operations	*i_op;
    struct super_block		*i_sb;
    struct wait_queue		i_wait;
    struct inode		*i_next;
    struct inode		*i_prev;
    struct inode		*i_mount;
    unsigned short		i_count;
    unsigned short		i_flags;
    unsigned char		i_lock;
    unsigned char		i_dirt;

    short			i_sem;

    union {
	struct pipe_inode_info	pipe_i;
	struct romfs_inode_info	romfs_i;
	struct socket		socket_i;
	void			*generic_i;
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

#include <linuxmt/minix_fs_sb.h>
#include <linuxmt/romfs_fs_sb.h>

struct super_block {
    kdev_t			s_dev;
    unsigned char		s_lock;

#ifdef BLOAT_FS
    unsigned char		s_rd_only;
#endif

    unsigned char		s_dirt;
    struct file_system_type	*s_type;
    struct super_operations	*s_op;
    unsigned short int		s_flags;

#ifdef BLOAT_FS
    __u32			s_magic;
    time_t			s_time;
#endif

    struct inode		*s_covered;
    struct inode		*s_mounted;
    struct wait_queue		s_wait;

    union {
	struct minix_sb_info	minix_sb;
	struct romfs_sb_info	romfs_sb;
	void			*generic_sbp;
    } u;
};

/*
 * This is the "filldir" function type, used by readdir() to let
 * the kernel specify what kind of dirent layout it wants to have.
 * This allows the kernel to read directories into kernel space or
 * to have different dirent layouts depending on the binary type.
 */

typedef int (*filldir_t) ();

struct file_operations {
    int				(*lseek) ();
    size_t			(*read)(struct inode *,struct file *,char *,size_t);
    size_t			(*write)(struct inode *,struct file *,char *,size_t);
    int 			(*readdir) ();
    int 			(*select) ();
    int 			(*ioctl) ();
    int 			(*open) ();
    void			(*release) ();

#ifdef BLOAT_FS
    int 			(*fsync) ();
    int 			(*check_media_change) ();
    int 			(*revalidate) ();
#endif

};

struct inode_operations {
    struct file_operations	*default_file_ops;
    int 			(*create) ();
    int 			(*lookup) ();
    int 			(*link) ();
    int 			(*unlink) ();
    int 			(*symlink) ();
    int 			(*mkdir) ();
    int 			(*rmdir) ();
    int 			(*mknod) ();
    int 			(*readlink) ();
    int 			(*follow_link) ();

#ifdef BLOAT_FS
    int 			(*bmap) ();
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

#ifdef BLOAT_FS
    void			(*statfs_kern) ();
				/* i8086 statfs goes to kernel, then user */
#endif

    int 			(*remount_fs) ();
};

struct file_system_type {
    struct super_block		*(*read_super) ();
    char			*name;

#ifdef BLOAT_FS
    int 			requires_dev;
#endif

};

extern int event;		/* Event counter */

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

extern struct inode_operations sock_inode_operations;

extern struct file_system_type *get_fs_type(char *);

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
extern int notify_change(struct inode *,struct iattr *);
extern int namei(char *,struct inode **,int,int);

#define lnamei(_a,_b) _namei(_a,NULL,0,_b)

extern int permission(struct inode *,int);

#ifdef BLOAT_FS

extern int get_write_access(struct inode *);
extern void put_write_access(struct inode *);

#else

#define get_write_access(_a)
#define put_write_access(_a)

#endif

extern __s16 link_count;

extern int open_namei(char *,int,int,struct inode **,struct inode *);
extern int do_mknod(char *,int,dev_t);
extern void iput(struct inode *);

extern struct inode *get_empty_inode(void);
extern struct inode *new_inode(struct inode *dir, __u16 mode);
extern void insert_inode_hash(struct inode *);
extern void clear_inode(struct inode *);
extern int open_filp(unsigned short, struct inode *, struct file **);
extern void close_filp(struct inode *, struct file *);

extern struct buffer_head *get_hash_table(kdev_t,block_t);
extern struct buffer_head *getblk(kdev_t,block_t);
extern struct buffer_head *readbuf(struct buffer_head *);

extern void ll_rw_blk(int,struct buffer_head *);
extern void ll_rw_page(void);
extern void ll_rw_swap_file(void);

extern void map_buffer(struct buffer_head *);
extern void unmap_buffer(struct buffer_head *);
extern void unmap_brelse(struct buffer_head *);
extern void print_bufmap_status(void);

extern void put_super(kdev_t);
extern kdev_t ROOT_DEV;

extern void show_buffers(void);
extern void mount_root(void);

extern int char_read(struct inode *,struct file *,char *,int);

#ifdef CONFIG_BLK_DEV_CHAR

extern size_t block_read(struct inode *,struct file *,char *,size_t);
extern size_t block_write(struct inode *,struct file *,char *,size_t);

#else

#define block_read NULL
#define block_write NULL

#endif

extern int fd_check(unsigned int,char *,size_t,int,struct file **);
extern void fs_init(void);

/*@-namechecks@*/

extern int _namei(char *,struct inode *,int,struct inode **);

/*@+namechecks@*/

extern int sys_dup(unsigned int);

extern struct buffer_head *bread(dev_t,block_t);

extern int get_unused_fd(struct file *);
extern int open_fd(int flags, struct inode *inode);

extern void mark_buffer_uptodate(struct buffer_head *,int);

#endif

#endif
