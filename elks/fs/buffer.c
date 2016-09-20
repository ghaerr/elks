#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/string.h>
#include <linuxmt/mm.h>
#include <linuxmt/locks.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>

#include <arch/system.h>
#include <arch/segment.h>
#include <arch/io.h>
#include <arch/irq.h>

static struct buffer_head buffers[NR_BUFFERS];
#ifdef DMA_ALN
static char bufmem[(NR_MAPBUFS+1)*BLOCK_SIZE-1];	/* L1 buffer area */
static char *bufmem_i;
#else
static char bufmem[NR_MAPBUFS][BLOCK_SIZE];	/* L1 buffer area */
#endif

/*
 *	STUBS for the buffer cache when we put it in
 */
/*static struct buffer_head *bh_chain = buffers; */
static struct buffer_head *bh_lru = buffers;
static struct buffer_head *bh_llru = NULL;

#if 0
struct wait_queue bufwait;	/* Wait for a free buffer */
#endif

#ifdef CONFIG_FS_EXTERNAL_BUFFER
static struct wait_queue bufmapwait;	/* Wait for a free L1 buffer area */
static struct buffer_head *bufmem_map[NR_MAPBUFS]; /* Array of bufmem's allocation */
static __u16 _buf_ds;			/* Segment(s?) of L2 buffer cache */
static int lastumap;
#endif

static void put_last_lru(register struct buffer_head *bh)
{
    register struct buffer_head *bhn;

    if (bh_llru != bh) {
	/*
	 *      Unhook
	 */
	bhn = bh->b_next_lru;
	if ((bhn->b_prev_lru = bh->b_prev_lru))
	    bh->b_prev_lru->b_next_lru = bhn;
	/*
	 *      Alter head
	 */
	if (bh == bh_lru)
	    bh_lru = bhn;
	/*
	 *      Put on lru end
	 */
	bh->b_next_lru = NULL;
	bh->b_prev_lru = bh_llru;
	bh_llru->b_next_lru = bh;
	bh_llru = bh;
    }
}

void buffer_init(void)
{
    register struct buffer_head *bh = buffers;
    register char *pi;

#ifdef DMA_ALN
    pi = (char *)((-(((int)kernel_ds << 4) + (int)bufmem)) & (BLOCK_SIZE - 1));
    bufmem_i = (char *)bufmem + (unsigned int)pi;
#endif
#ifdef CONFIG_FS_EXTERNAL_BUFFER
    _buf_ds = mm_alloc(NR_BUFFERS * 0x40);
    pi = (char *)0;
    do {
	bufmem_map[(unsigned int)pi] = NULL;
    } while ((unsigned int)(++pi) < NR_MAPBUFS);
#endif

    do {
#ifdef CONFIG_FS_EXTERNAL_BUFFER
	bh->b_data = 0;		/* L1 buffer cache is reserved! */
	bh->b_mapcount = 0;
	bh->b_num = (unsigned int)pi;		/* Used to compute L2 location */
#else
#ifdef DMA_ALN
	bh->b_data = bufmem_i + ((unsigned int)pi << BLOCK_SIZE_BITS);
#else
	bh->b_data = (char *)bufmem + ((unsigned int)pi << BLOCK_SIZE_BITS);
#endif
#endif
	if ((unsigned int)pi == NR_BUFFERS - 1) {
	    bh->b_next_lru = NULL;
/*	    bh->b_next = NULL; */
	    bh_llru = bh;
	} else {
	    bh->b_prev_lru = bh - 1;
	    bh->b_next_lru = bh + 1;
/*	    bh->b_next = bh + 1; */
	}
	pi++;
    } while(++bh < &buffers[NR_BUFFERS]);
    buffers[0].b_prev_lru = NULL;
}

/*
 *	Wait on a buffer
 */

void wait_on_buffer(register struct buffer_head *bh)
{
    register __ptask currentp = current;

    if (buffer_locked(bh)) {

	bh->b_count++;

	wait_set(&bh->b_wait);
	currentp->state = TASK_UNINTERRUPTIBLE;
	while (buffer_locked(bh))
	    schedule();
	currentp->state = TASK_RUNNING;
	wait_clear(&bh->b_wait);

	bh->b_count--;
    }
}

void lock_buffer(register struct buffer_head *bh)
{
    wait_on_buffer(bh);
    bh->b_lock = 1;
    map_buffer(bh);
}

void unlock_buffer(register struct buffer_head *bh)
{
    bh->b_lock = 0;
    unmap_buffer(bh);
    wake_up(&bh->b_wait);
}

void invalidate_buffers(kdev_t dev)
{
    register struct buffer_head *bh = bh_llru;

    do {
	if (bh->b_dev != dev)
	    continue;
	wait_on_buffer(bh);
	if (bh->b_dev != dev)
	    continue;
	if (bh->b_count)
	    continue;
	bh->b_uptodate = 0;
	bh->b_dirty = 0;
	bh->b_lock = 0;
    } while ((bh = bh->b_prev_lru) != NULL);
}

static void sync_buffers(kdev_t dev, int wait)
{
    register struct buffer_head *bh = bh_llru;

    do {
	if (dev && bh->b_dev != dev)
	    continue;
	/*
	 *      Skip clean buffers.
	 */
	if (buffer_clean(bh))
	    continue;
	/*
	 *      Locked buffers..
	 *
	 *      Buffer is locked; skip it unless wait is requested
	 *      AND pass > 0.
	 */
	if (buffer_locked(bh) && wait)
	    continue;
	else
	    wait_on_buffer(bh);
	/*
	 *      Do the stuff
	 */
	bh->b_count++;
	ll_rw_blk(WRITE, bh);
	bh->b_count--;
    } while ((bh = bh->b_prev_lru) != NULL);
}

void fsync_dev(kdev_t dev)
{
    sync_buffers(dev, 0);
    sync_supers(dev);
    sync_inodes(dev);
    sync_buffers(dev, 1);
}

void sync_dev(kdev_t dev)
{
    sync_buffers(dev, 0);
    sync_supers(dev);
    sync_inodes(dev);
    sync_buffers(dev, 0);
}

int sys_sync(void)
{
    fsync_dev(0);
    return 0;
}

static struct buffer_head *get_free_buffer(void)
{
    register struct buffer_head *bh;

    for (;;) {
	bh = bh_lru;
	do {
#ifdef CONFIG_FS_EXTERNAL_BUFFER
	    if (bh->b_count == 0 && !bh->b_dirty && !bh->b_lock && !bh->b_data) {
#else
	    if (bh->b_count == 0 && !bh->b_dirty && !bh->b_lock) {
#endif
		put_last_lru(bh);
		return bh;
	    }
	} while ((bh = bh->b_next_lru) != NULL);
#if 0
	fsync_dev(0);
	/* This causes a sleep until another process brelse's */
	sleep_on(&bufwait);
#endif
	sync_buffers(0, 0);
    }
}

/*
 * Release a buffer head
 */

void __brelse(register struct buffer_head *buf)
{
    wait_on_buffer(buf);

    if (buf->b_count) {
	buf->b_count--;
#if 0
	wake_up(&bufwait);
#endif
    } else
	panic("brelse");
}

/*
 * bforget() is like brelse(), except it removes the buffer
 * data validity.
 */
#if 0
void __bforget(struct buffer_head *buf)
{
    wait_on_buffer(buf);
    buf->b_dirty = 0;
    buf->b_count--;
    buf->b_dev = NODEV;
    wake_up(&bufwait);
}
#endif

static struct buffer_head *find_buffer(kdev_t dev, block_t block)
{
    register struct buffer_head *bh = bh_llru;

    do {
	if (bh->b_blocknr == block && bh->b_dev == dev)
	    break;
    } while ((bh = bh->b_prev_lru) != NULL);
    return bh;
}

struct buffer_head *get_hash_table(kdev_t dev, block_t block)
{
    register struct buffer_head *bh;

    while ((bh = find_buffer(dev, block))) {
	bh->b_count++;
	wait_on_buffer(bh);
	if (bh->b_dev == dev && bh->b_blocknr == block)
	    break;
	bh->b_count--;
    }
    return bh;
}

/*
 * Ok, this is getblk, and it isn't very clear, again to hinder
 * race-conditions. Most of the code is seldom used, (ie repeating),
 * so it should be much more efficient than it looks.
 *
 * The algorithm is changed: hopefully better, and an elusive bug removed.
 *
 * 14.02.92: changed it to sync dirty buffers a bit: better performance
 * when the filesystem starts to get full of dirty blocks (I hope).
 */

struct buffer_head *getblk(kdev_t dev, block_t block)
{
    register struct buffer_head *bh;


    /* If there are too many dirty buffers, we wake up the update process
     * now so as to ensure that there are still clean buffers available
     * for user processes to use (and dirty) */

    do {
	bh = get_hash_table(dev, block);
	if (bh != NULL) {
	    if (buffer_clean(bh) && buffer_uptodate(bh))
		put_last_lru(bh);
	    return bh;
	}

	/* I think the following check is redundant
	 * So I will remove it for now
	 */

    } while (find_buffer(dev, block));

    /*
     *      Create a buffer for this job.
     */
    bh = get_free_buffer();

/* OK, FINALLY we know that this buffer is the only one of its kind,
 * and that it's unused (b_count=0), unlocked (buffer_locked=0), and clean
 */

    bh->b_count = 1;
    bh->b_uptodate = 0;
    bh->b_dev = dev;
    bh->b_blocknr = block;
    bh->b_seg = kernel_ds;

    return bh;
}

/* Turns out both minix_bread and bread do this, so I made this a function
 * of it's own... */

struct buffer_head *readbuf(register struct buffer_head *bh)
{
    if (!buffer_uptodate(bh)) {
	ll_rw_blk(READ, bh);
	wait_on_buffer(bh);
	if (!buffer_uptodate(bh)) {
	    brelse(bh);
	    bh = NULL;
	}
    }
    return bh;
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It returns NULL if the block was unreadable.
 */

struct buffer_head *bread(kdev_t dev, block_t block)
{
    return readbuf(getblk(dev, block));
}

#if 0

/* NOTHING is using breada at this point, so I can pull it out... Chad */

struct buffer_head *breada(kdev_t dev,
			   block_t block,
			   int bufsize,
			   unsigned int pos, unsigned int filesize)
{
    register struct buffer_head *bh, *bha;
    int i, j;

    if (pos >= filesize)
	return NULL;

    if (block < 0)
	return NULL;
    bh = getblk(dev, block);

    if (buffer_uptodate(bh))
	return bh;

    bha = getblk(dev, block + 1);
    if (buffer_uptodate(bha)) {
	brelse(bha);
	bha = NULL;
    } else {
	/* Request the read for these buffers, and then release them */
	ll_rw_blk(READ, bha);
	brelse(bha);
    }
    /* Wait for this buffer, and then continue on */
    wait_on_buffer(bh);
    if (buffer_uptodate(bh))
	return bh;
    brelse(bh);
    return NULL;
}

#endif

void mark_buffer_uptodate(struct buffer_head *bh, int on)
{
    flag_t flags;

    save_flags(flags);
    clr_irq();
    bh->b_uptodate = on;
    restore_flags(flags);
}

#ifdef CONFIG_FS_EXTERNAL_BUFFER

/* map_buffer forces a buffer into L1 buffer space. It will freeze forever
 * before failing, so it can return void.  This is mostly 8086 dependant,
 * although the interface is not. */

void map_buffer(register struct buffer_head *bh)
{
    register char *pi;

    /* If buffer is already mapped, just increase the refcount and return
     */
    debug2("mapping buffer %d (%d)\n", bh->b_num, bh->b_mapcount);

    if (bh->b_data /*|| bh->b_seg != kernel_ds*/) {

#ifdef DEBUG
	if (!bh->b_mapcount) {
	    debug("BUFMAP: Buffer %d (block %d) `remapped' into L1.\n",
		  bh->b_num, bh->b_blocknr);
	}
#endif

	bh->b_mapcount++;
	return;
    }

    /* else keep trying till we succeed */
    for (;;) {
	/* First check for the trivial case */
	pi = (char *)0;
	do {
	    if (!bufmem_map[(int)pi])
		goto do_map_buffer;
	} while((int)(++pi) < NR_MAPBUFS);

	/* Now, we check for a mapped buffer with no count and then
	 * hopefully find one to send back to L2 */
	pi = (char *)lastumap;
	goto init_while;
	do {
	    debug1("BUFMAP: trying slot %d\n", (int)pi);

	    if (!bufmem_map[(int)pi]->b_mapcount) {
		debug1("BUFMAP: Buffer %d unmapped from L1\n",
			       bufmem_map[(int)pi]->b_num);
		/* Now unmap it */
		fmemcpy(_buf_ds, (__u16)(bufmem_map[(int)pi]->b_num << BLOCK_SIZE_BITS),
			kernel_ds, (__u16) bufmem_map[(int)pi]->b_data, BLOCK_SIZE);
		bufmem_map[(int)pi]->b_data = 0;
		/* success */
		lastumap = (int)pi;
		goto do_map_buffer;
	    }
	  init_while:
	    if((char *)(++pi) >= NR_MAPBUFS)
		pi = (char *)0;
	} while ((int)pi != lastumap);

	/* previous loop failed */
	/* The last case is to wait until unmap gets a b_mapcount down to 0 */
	debug1("BUFMAP: buffer #%d waiting on L1 slot\n", bh->b_num);
	sleep_on(&bufmapwait);
	debug("BUFMAP: wait queue woken up...\n");
    }
  do_map_buffer:
    /* We can just map here! */
    bufmem_map[(int)pi] = bh;
#ifdef DMA_ALN
    bh->b_data = bufmem_i + ((int)pi << BLOCK_SIZE_BITS);
#else
    bh->b_data = (char *)bufmem + ((int)pi << BLOCK_SIZE_BITS);
#endif
    bh->b_mapcount++;
    if (bh->b_uptodate)
	fmemcpy(kernel_ds, (__u16) bh->b_data, _buf_ds,
	    (__u16) (bh->b_num << BLOCK_SIZE_BITS), BLOCK_SIZE);
    debug3("BUFMAP: Buffer %d (block %d) mapped into L1 slot %d.\n",
	bh->b_num, bh->b_blocknr, (int)pi);
}

/* unmap_buffer decreases bh->b_mapcount, and wakes up anyone waiting over
 * in map_buffer if it's decremented to 0... this is a bit of a misnomer,
 * since the unmapping is actually done in map_buffer to prevent frivoulous
 * unmaps if possible... */

void unmap_buffer(register struct buffer_head *bh)
{
    if (bh) {
	debug1("unmapping buffer %d\n", bh->b_num);
	if (bh->b_mapcount <= 0) {
	    printk("unmap_buffer: buffer #%x's b_mapcount<=0 already\n",
		   bh->b_num);
	    bh->b_mapcount = 0;
	} else if (!(--bh->b_mapcount)) {
		debug1("BUFMAP: buffer %d released from L1.\n", bh->b_num);
		wake_up(&bufmapwait);
	}
    }
}

void unmap_brelse(register struct buffer_head *bh)
{
    if (bh) {
	unmap_buffer(bh);
	__brelse(bh);
    }
}
#endif

/* This function prints the status of the L1 mappings... */

#if 0				/* Currently unused */
void print_bufmap_status(void)
{
    int i;

    printk("Current L1 buffer cache mappings :\n");
    printk("L1 slot / Buffer # / Reference Count\n");
    for (i = 0; i < NR_MAPBUFS; i++) {
	if (bufmem_map[i]) {
	    printk("%u / %u / %u\n", i, bufmem_map[i]->b_num,
		   bufmem_map[i]->b_mapcount);
	}
    }
}
#endif
