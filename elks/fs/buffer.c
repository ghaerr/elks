#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/string.h>
#include <linuxmt/mm.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>

#include <arch/system.h>
#include <arch/segment.h>
#include <arch/io.h>
#include <arch/irq.h>

// Number of external buffers

#define NR_EXT_BUFFERS 64

// Number of internal buffers
// used to "map" external buffers

#ifdef CONFIG_FS_FAT
#define NR_MAPBUFS  12
#else
#define NR_MAPBUFS  8
#endif

// Number of internal buffers

#ifdef CONFIG_FS_EXTERNAL_BUFFER
#define NR_BUFFERS NR_EXT_BUFFERS
#else
#define NR_BUFFERS NR_MAPBUFS
#endif

// Buffer heads (internal or external)

static struct buffer_head buffers[NR_BUFFERS];

// Internal buffers

static char bufmem[NR_MAPBUFS][BLOCK_SIZE];	/* L1 buffer area */

/*
 *	STUBS for the buffer cache when we put it in
 */
/*static struct buffer_head *bh_chain = buffers; */
static struct buffer_head *bh_lru = buffers;
static struct buffer_head *bh_llru = buffers;

// External buffers are allocated in one big 64K segment
// in the global memory (BLOCK_SIZE * NR_EXT_BUFFERS)

#ifdef CONFIG_FS_EXTERNAL_BUFFER
static struct wait_queue bufmapwait;	/* Wait for a free L1 buffer area */
static struct buffer_head *bufmem_map[NR_MAPBUFS]; /* Array of bufmem's allocation */
static __u16 _buf_ds;			/* Segment(s?) of L2 buffer cache */
static int lastumap;
#endif

/* Uncomment next line to debug free buffer_head count */
/*#define DEBUG_FREE_BH_COUNT */

#ifdef DEBUG_FREE_BH_COUNT
static int nr_free_bh = NR_BUFFERS;
#define DCR_COUNT(bh) if (!(--bh->b_count))nr_free_bh++
#define INR_COUNT(bh) if (!(bh->b_count++))nr_free_bh--
#define CLR_COUNT(bh) if (bh->b_count)nr_free_bh++
#define SET_COUNT(bh) if (--nr_free_bh < 0) { \
	printk("VFS: get_free_buffer: bad free buffer head count.\n"); \
	nr_free_bh = 0; \
    }
#else
#define DCR_COUNT(bh) (bh->b_count--)
#define INR_COUNT(bh) (bh->b_count++)
#define CLR_COUNT(bh)
#define SET_COUNT(bh)
#endif

static void put_last_lru(register struct buffer_head *bh)
{
    register struct buffer_head *bhn;

    if (bh_llru != bh) {

	bhn = bh->b_next_lru;
	if ((bhn->b_prev_lru = bh->b_prev_lru))	/* Unhook */
	    bh->b_prev_lru->b_next_lru = bhn;
	else					/* Alter head */
	    bh_lru = bhn;
	/*
	 *      Put on lru end
	 */
	bh->b_next_lru = NULL;
	(bh->b_prev_lru = bh_llru)->b_next_lru = bh;
	bh_llru = bh;
    }
}

void buffer_init(void)
{

#ifdef CONFIG_FS_EXTERNAL_BUFFER
	// TODO: allocate buffer one by one in global memory
    _buf_ds = mm_alloc(NR_BUFFERS << (BLOCK_SIZE_BITS - 4));
    unsigned int i = NR_MAPBUFS;
    do {
	bufmem_map[--i] = NULL;
    } while (i > 0);
#else
	char * p = bufmem;
#endif

    struct buffer_head *bh = buffers;

    goto buf_init;
    do {
	bh->b_next_lru = bh->b_prev_lru = bh;
	put_last_lru(bh);
      buf_init:

#ifdef CONFIG_FS_EXTERNAL_BUFFER
	bh->b_data = (char *)0;			/* L1 buffer cache is reserved! */
	bh->b_mapcount = 0;
	bh->b_num = i++;	/* Used to compute L2 location */
#else
	bh->b_data = p;
	p += BLOCK_SIZE;
#endif

    } while (++bh < &buffers[NR_BUFFERS]);
}

/*
 *	Wait on a buffer
 */

void wait_on_buffer(register struct buffer_head *bh)
{
    while (buffer_locked(bh)) {
	INR_COUNT(bh);
	sleep_on(&bh->b_wait);
	DCR_COUNT(bh);
    }
}

// Lock buffer in memory
// when driver is working on it
// or client is reading from / writing to it

// TODO: replace by a 'lock_t'

void lock_buffer(register struct buffer_head *bh)
{
    wait_on_buffer(bh);
    bh->b_lock = 1;
}

void unlock_buffer(register struct buffer_head *bh)
{
    bh->b_lock = 0;
    wake_up(&bh->b_wait);
}

void invalidate_buffers(kdev_t dev)
{
    register struct buffer_head *bh = bh_llru;

    do {
	if (bh->b_dev != dev) continue;
	wait_on_buffer(bh);
/*	if (bh->b_dev != dev)*/	/* This will never happen */
/*	    continue;*/
	if (bh->b_count) continue;
	bh->b_uptodate = 0;
	bh->b_dirty = 0;
	unlock_buffer(bh);
    } while ((bh = bh->b_prev_lru) != NULL);
}

static void sync_buffers(kdev_t dev, int wait)
{
    register struct buffer_head *bh = bh_llru;

    do {
	/*
	 *      Skip clean buffers.
	 */
	if ((dev && (bh->b_dev != dev)) || (buffer_clean(bh))) continue;
	/*
	 *      Locked buffers..
	 *
	 *      If buffer is locked; skip it unless wait is requested
	 *      AND pass > 0.
	 */
	if (buffer_locked(bh)) {
	    if (!wait) continue;
	    wait_on_buffer(bh);
	}
	/*
	 *      Do the stuff
	 */
	INR_COUNT(bh);
	ll_rw_blk(WRITE, bh);
	DCR_COUNT(bh);
    } while ((bh = bh->b_prev_lru) != NULL);
}

static struct buffer_head *get_free_buffer(void)
{
    register struct buffer_head *bh;

    bh = bh_lru;
#ifdef CONFIG_FS_EXTERNAL_BUFFER
    while (bh->b_count || bh->b_dirty || buffer_locked (bh) || bh->b_data) {
#else
    while (bh->b_count || bh->b_dirty || buffer_locked (bh)) {
#endif
	if ((bh = bh->b_next_lru) == NULL) {
	    sync_buffers(0, 0);
	    bh = bh_lru;
	}
    }
    put_last_lru(bh);
    bh->b_uptodate = 0;
    bh->b_count = 1;
    SET_COUNT(bh)
    return bh;
}

/*
 * Release a buffer head
 */

void __brelse(register struct buffer_head *buf)
{
    wait_on_buffer(buf);

    if (buf->b_count <= 0) panic("brelse");
#if 0
    if (!--buf->b_count)
	wake_up(&bufwait);
#else
    DCR_COUNT(buf);
#endif
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
    DCR_COUNT(buf);
    buf->b_dev = NODEV;
    wake_up(&bufwait);
}
#endif

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

static struct buffer_head *find_buffer(kdev_t dev, block_t block)
{
    register struct buffer_head *bh = bh_llru;

    do {
	if (bh->b_blocknr == block && bh->b_dev == dev) break;
    } while ((bh = bh->b_prev_lru) != NULL);
    return bh;
}

struct buffer_head *get_hash_table(kdev_t dev, block_t block)
{
    register struct buffer_head *bh;

    if ((bh = find_buffer(dev, block)) != NULL) {
	INR_COUNT(bh);
	wait_on_buffer(bh);
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
    register struct buffer_head *n_bh;

    /* If there are too many dirty buffers, we wake up the update process
     * now so as to ensure that there are still clean buffers available
     * for user processes to use (and dirty) */

    n_bh = NULL;
    goto start;
    do {
	/*
	 * Block not found. Create a buffer for this job.
	 */
	n_bh = get_free_buffer();	/* This function may sleep and someone else */
      start:				/* can create the block */
	if ((bh = find_buffer(dev, block)) != NULL) goto found_it;
    } while (n_bh == NULL);
    bh = n_bh;				/* Block not found, use the new buffer */
/* OK, FINALLY we know that this buffer is the only one of its kind,
 * and that it's unused (b_count=0), unlocked (buffer_locked=0), and clean
 */
    bh->b_dev = dev;
    bh->b_blocknr = block;
    bh->b_seg = kernel_ds;
    goto return_it;

  found_it:
    if (n_bh != NULL) {
	CLR_COUNT(n_bh)
	n_bh->b_count = 0;	/* Release previously created buffer head */
    }
    INR_COUNT(bh);
    wait_on_buffer(bh);
    if (buffer_clean(bh) && buffer_uptodate(bh))
	put_last_lru(bh);

  return_it:
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

#ifdef CONFIG_FS_EXTERNAL_BUFFER

/* map_buffer forces a buffer into L1 buffer space. It will freeze forever
 * before failing, so it can return void.  This is mostly 8086 dependant,
 * although the interface is not. */

void map_buffer(register struct buffer_head *bh)
{
    struct buffer_head *bmap;
    register char *pi;

    /* If buffer is already mapped, just increase the refcount and return
     */
    debug2("mapping buffer %u (%d)\n", bh->b_num, bh->b_mapcount);

    if (bh->b_data /*|| bh->b_seg != kernel_ds*/) {

#ifdef DEBUG
	if (!bh->b_mapcount) {
	    debug("BUFMAP: Buffer %u (block %u) `remapped' into L1.\n",
		  bh->b_num, bh->b_blocknr);
	}
#endif

	goto end_map_buffer;
    }

    pi = (char *)lastumap;
    /* Keep trying till we succeed */
    do {
	if ((int)(++pi) >= NR_MAPBUFS) pi = (char *)0;
	debug1("BUFMAP: trying slot %d\n", (int)pi);

	/* First check for the tivial case, to avoid dereferencing a null pointer */
	if (!(bmap = bufmem_map[(int)pi])) break;

	/* Now, we check for a mapped buffer with no count and then
	 * hopefully find one to send back to L2 */
	if (!bmap->b_mapcount) {
	    debug1("BUFMAP: Buffer %u unmapped from L1\n",
		    bmap->b_num);
	    /* Now unmap it */
		fmemcpyb((byte_t *) (bmap->b_num << BLOCK_SIZE_BITS), _buf_ds,
			(byte_t *) bmap->b_data, kernel_ds, BLOCK_SIZE);
	    bmap->b_data = 0;
	    /* success */
	    break;
	}
	if ((int)pi == lastumap) {
	    /* The scan of all buffers failed */
	    /* The last resort is to wait until unmap gets a b_mapcount down to 0 */
	    /* replaced debug1 with printk here to indicate where it gets stuck - Georg P. */
	    debug1("BUFMAP: buffer #%u waiting on L1 slot\n", bh->b_num);	/*Back to debug1*/
	    sleep_on(&bufmapwait);
	    debug("BUFMAP: wait queue woken up...\n");
	}
    } while (1);

    lastumap = (int)pi;
    /* We can just map here! */
    bufmem_map[(int)pi] = bh;
    bh->b_data = (char *)bufmem + ((int)pi << BLOCK_SIZE_BITS);
    if (bh->b_uptodate)
	fmemcpyb ((byte_t *) bh->b_data, kernel_ds,
		(byte_t *) (bh->b_num << BLOCK_SIZE_BITS), _buf_ds, BLOCK_SIZE);
    debug3("BUFMAP: Buffer %u (block %u) mapped into L1 slot %d.\n",
	bh->b_num, bh->b_blocknr, (int)pi);
  end_map_buffer:
    bh->b_mapcount++;
}

/* unmap_buffer decreases bh->b_mapcount, and wakes up anyone waiting over
 * in map_buffer if it's decremented to 0... this is a bit of a misnomer,
 * since the unmapping is actually done in map_buffer to prevent frivoulous
 * unmaps if possible... */

void unmap_buffer(register struct buffer_head *bh)
{
    if (bh) {
	debug1("unmapping buffer %u\n", bh->b_num);
	if (bh->b_mapcount <= 0) {
	    printk("unmap_buffer: buffer #%u's b_mapcount<=0 already\n",
		   bh->b_num);
	    bh->b_mapcount = 0;
	} else if (!(--bh->b_mapcount)) {
	    debug1("BUFMAP: buffer %u released from L1.\n", bh->b_num);
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
