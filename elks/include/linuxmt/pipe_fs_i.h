#ifndef __LINUXMT_PIPE_FS_I_H
#define __LINUXMT_PIPE_FS_I_H

#include <linuxmt/chqueue.h>

struct pipe_inode_info {
    struct ch_queue q;
    unsigned int lock;
    unsigned int rd_openers;
    unsigned int wr_openers;
    unsigned int readers;
    unsigned int writers;
};

#define PIPE_WAIT(inode)	((inode)->u.pipe_i.q.wait)
#define PIPE_BASE(inode)	((inode)->u.pipe_i.q.base)
#define PIPE_HEAD(inode)	((inode)->u.pipe_i.q.head)
#define PIPE_TAIL(inode)	((inode)->u.pipe_i.q.tail)
#define PIPE_LEN(inode)		((inode)->u.pipe_i.q.len)
#define PIPE_SIZE(inode)	((inode)->u.pipe_i.q.size)
#define PIPE_LOCK(inode)	((inode)->u.pipe_i.lock)
#define PIPE_RD_OPENERS(inode)	((inode)->u.pipe_i.rd_openers)
#define PIPE_WR_OPENERS(inode)	((inode)->u.pipe_i.wr_openers)
#define PIPE_READERS(inode)	((inode)->u.pipe_i.readers)
#define PIPE_WRITERS(inode)	((inode)->u.pipe_i.writers)

#define PIPE_EMPTY(inode)	(PIPE_LEN(inode) == 0)
#define PIPE_FULL(inode)	(PIPE_LEN(inode) == PIPE_SIZE(inode))
#define PIPE_FREE(inode)	(PIPE_SIZE(inode) - PIPE_LEN(inode))
#define PIPE_MAX_RCHUNK(inode)	(PIPE_SIZE(inode) - PIPE_TAIL(inode))
#define PIPE_MAX_WCHUNK(inode)	(PIPE_SIZE(inode) - PIPE_HEAD(inode))

#endif
