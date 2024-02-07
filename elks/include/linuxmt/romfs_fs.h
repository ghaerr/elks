#ifndef _LINUXMT_ROMFS_FS_H
#define _LINUXMT_ROMFS_FS_H

/* ROMFS - A tiny read-only filesystem in memory */

#define ROMFS_MAGIC 0x7275
#define ROMFS_MAGIC_STR "ROMFS"
#define ROMFS_MAGIC_LEN 6  /* even length for word compare */

#define ROMFS_NAME_MAX 14  /* was 255, made compatible with MINIX*/


/* In-memory superblock */
/* Even aligned and size for word copy */
/* TODO: common declaration with mkromfs */

struct romfs_super_mem {
	byte_t magic [6];
	word_t ssize;    /* size of super block */
	word_t isize;    /* size of inode */
	word_t icount;   /* number of inodes */
};

/* In-memory inode */
/* Even aligned and size for word copy */
/* TODO: common declaration with mkromfs */

struct romfs_inode_mem {
	word_t offset;  /* offset in paragraphs */
	word_t size;    /* size in bytes */
	word_t flags;
};

#define ROMFS_TYPE 7
#define ROMFH_REG 0
#define ROMFH_DIR 1
#define ROMFH_CHR 2
#define ROMFH_BLK 3
#define ROMFH_LNK 4

struct romfs_super_info {
       word_t ssize;   /* size of superblock */
       word_t isize;   /* size of inode */
       word_t icount;  /* number of inodes */
};

struct romfs_inode_info {
       word_t seg;     /* inode segment */
};

#endif
