/* ROMFS - A tiny filesystem in ROM */

#ifndef _LINUXMT_ROMFS_FS_H
#define _LINUXMT_ROMFS_FS_H

#define ROMBSIZE BLOCK_SIZE
#define ROMBSBITS BLOCK_SIZE_BITS
#define ROMBMASK (ROMBSIZE-1)
#define ROMFS_MAGIC 0x7275

#define ROMFS_MAXFN 128

/*@-namechecks@*/

#define __mkw(h,l) (((h)&0x00ffL)<< 8|((l)&0x00ffL))
#define __mkl(h,l) (((h)&0xffffL)<<16|((l)&0xffffL))
#define __mk4(a,b,c,d) htonl((unsigned long int)__mkl(__mkw(a,b),__mkw(c,d)))



/* In-memory superblock */
/* TODO: common declaration with mkromfs */

struct romfs_superblock_s
	{
	byte_t magic [6];
	word_t ssize;    /* size of super block */
	word_t isize;    /* size of inode */
	word_t icount;   /* number of inodes */
	};

/* In-memory inode */
/* TODO: common declaration with mkromfs */

struct romfs_inode_s
	{
	word_t offset;  /* offset in paragraphs */
	word_t size;    /* size in bytes */
	word_t flags;
	};

#define ROMFH_TYPE 3
#define ROMFH_REG 0
#define ROMFH_DIR 1
#define ROMFH_CHR 2
#define ROMFH_BLK 3

#endif  /* !_LINUXMT_ROMFS_FS_H */
