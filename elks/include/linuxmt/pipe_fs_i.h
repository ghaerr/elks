#ifndef _LINUX_PIPE_FS_I_H
#define _LINUX_PIPE_FS_I_H

struct pipe_inode_info {
	struct wait_queue * wait;
	char * base;
	unsigned int start;
	unsigned int len;
	unsigned int lock;
	unsigned int rd_openers;
	unsigned int wr_openers;
	unsigned int readers;
	unsigned int writers;
};

#define PAGE_SIZE 512
#define PIPE_BUF 512

#define PIPE_WAIT(inode)	((inode).u.pipe_i.wait)
#define PIPE_BASE(inode)	((inode).u.pipe_i.base)
#define PIPE_START(inode)	((inode).u.pipe_i.start)
#define PIPE_LEN(inode)		((inode).u.pipe_i.len)
#define PIPE_RD_OPENERS(inode)	((inode).u.pipe_i.rd_openers)
#define PIPE_WR_OPENERS(inode)	((inode).u.pipe_i.wr_openers)
#define PIPE_READERS(inode)	((inode).u.pipe_i.readers)
#define PIPE_WRITERS(inode)	((inode).u.pipe_i.writers)
#define PIPE_LOCK(inode)	((inode).u.pipe_i.lock)
#define PIPE_SIZE(inode)	PIPE_LEN(inode)

#define PIPE_EMPTY(inode)	(PIPE_SIZE(inode)==0)
#define PIPE_FULL(inode)	(PIPE_SIZE(inode)==PIPE_BUF)
#define PIPE_FREE(inode)	(PIPE_BUF - PIPE_LEN(inode))
#define PIPE_END(inode)		((PIPE_START(inode)+PIPE_LEN(inode))&\
							   (PIPE_BUF-1))
#define PIPE_MAX_RCHUNK(inode)	(PIPE_BUF - PIPE_START(inode))
#define PIPE_MAX_WCHUNK(inode)	(PIPE_BUF - PIPE_END(inode))

#endif
