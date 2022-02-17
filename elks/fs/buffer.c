#include <linuxmt/types.h>
#include <linuxmt/init.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/string.h>
#include <linuxmt/mm.h>
#include <linuxmt/heap.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>

#include <arch/system.h>
#include <arch/segment.h>
#include <arch/io.h>
#include <arch/irq.h>

/*
 * Kernel buffer management.
 */

/* Number of internal L1 buffers,
   used to map/copy external L2 buffers to/from kernel data segment */
#ifdef CONFIG_FS_FAT
#define NR_MAPBUFS  12
#else
#define NR_MAPBUFS  8
#endif

int boot_bufs;		/* /bootopts # buffers override */

/* Buffer heads: local heap allocated */
static struct buffer_head *buffer_heads;

#ifdef CONFIG_FAR_BUFHEADS
static ext_buffer_head *ext_buffer_heads;

/* convert a buffer_head ptr to ext_buffer_head ptr */
ext_buffer_head *EBH(struct buffer_head *bh)
{
	int idx = bh - buffer_heads;
	return ext_buffer_heads + idx;
}

/* functions for buffer_head points called outside of buffer.c */
void mark_buffer_dirty(struct buffer_head *bh)     { EBH(bh)->b_dirty = 1; }
void mark_buffer_clean(struct buffer_head *bh)     { EBH(bh)->b_dirty = 0; }
ramdesc_t buffer_seg(struct buffer_head *bh)       { return EBH(bh)->b_seg; }
unsigned char buffer_count(struct buffer_head *bh) { return EBH(bh)->b_count; }
block32_t buffer_blocknr(struct buffer_head *bh)   { return EBH(bh)->b_blocknr; }
kdev_t buffer_dev(struct buffer_head *bh)          { return EBH(bh)->b_dev; }

#endif /* CONFIG_FAR_BUFHEADS */

/* Internal L1 buffers, must be kernel DS addressable */
static char L1buf[NR_MAPBUFS][BLOCK_SIZE];

/* Buffer cache */
static struct buffer_head *bh_lru;
static struct buffer_head *bh_llru;
static struct buffer_head *bh_next;

/*
 * External L2 buffers are allocated within main or xms memory segments.
 * If CONFIG_FS_XMS_BUFFER is set and unreal mode and A20 gate can be enabled,
 *   extended/xms memory will be used and CONFIG_FS_NR_EXT_BUFFERS ignored.
 * Total extended/xms memory would be (BLOCK_SIZE * CONFIG_FS_NR_XMS_BUFFERS).
 * Otherwise, total main memory used is (BLOCK_SIZE * CONFIG_FS_NR_EXT_BUFFERS),
 * each main memory segment being up to 64K in size for segment register addressing
 *
 * Number of external/main (L2) buffers specified in config by CONFIG_FS_NR_EXT_BUFFERS
 * Number of extended/xms  (L2) buffers specified in config by CONFIG_FS_NR_XMS_BUFFERS
 */
#if defined(CONFIG_FS_EXTERNAL_BUFFER) || defined(CONFIG_FS_XMS_BUFFER)
static struct buffer_head *L1map[NR_MAPBUFS];	/* L1 indexed pointer to L2 buffer */
static struct wait_queue L1wait;		/* Wait for a free L1 buffer area */
static int lastL1map;
#endif

/* Uncomment next line to debug free buffer_head count */
/*#define DEBUG_FREE_BH_COUNT */

#ifdef DEBUG_FREE_BH_COUNT
static int nr_free_bh = NR_BUFFERS;
#define DCR_COUNT(bh) if(!(--bh->b_count))nr_free_bh++
#define INR_COUNT(bh) if(!(bh->b_count++))nr_free_bh--
#define CLR_COUNT(bh) if(bh->b_count)nr_free_bh++
#define SET_COUNT(bh) if(--nr_free_bh < 0) { \
	printk("VFS: get_free_buffer: bad free buffer head count.\n"); \
	nr_free_bh = 0; }
#else
#define DCR_COUNT(bh) (bh->b_count--)
#define INR_COUNT(bh) (bh->b_count++)
#define CLR_COUNT(bh)
#define SET_COUNT(bh)
#endif

#define buf_num(bh)	((bh) - buffer_heads)	/* buffer number, for debugging */

static void put_last_lru(struct buffer_head *bh)
{
    ext_buffer_head *ebh = EBH(bh);

    if (bh_llru != bh) {
	struct buffer_head *bhn = ebh->b_next_lru;
	ext_buffer_head *ebhn = EBH(bhn);

	if ((ebhn->b_prev_lru = ebh->b_prev_lru))	/* Unhook */
	    EBH(ebh->b_prev_lru)->b_next_lru = bhn;
	else					/* Alter head */
	    bh_lru = bhn;
	/*
	 *      Put on lru end
	 */
	ebh->b_next_lru = NULL;
	EBH(ebh->b_prev_lru = bh_llru)->b_next_lru = bh;
	bh_llru = bh;
    }
}

static void add_buffers(int nbufs, char *buf, ramdesc_t seg)
{
    struct buffer_head *bh;
    int n = 0;

    for (bh = bh_next; n < nbufs; n++, bh = ++bh_next) {
	ext_buffer_head *ebh = EBH(bh);

	if (bh != buffer_heads) {
	    ebh->b_next_lru = ebh->b_prev_lru = bh;
	    put_last_lru(bh);
	}

#if defined(CONFIG_FS_EXTERNAL_BUFFER) || defined(CONFIG_FS_XMS_BUFFER)
	ebh->b_seg = ebh->b_ds = seg;
	//bh->b_mapcount = 0;
	//bh->b_data = (char *)0;	/* not in L1 cache*/
	ebh->b_L2data = (char *)((n & 63) << BLOCK_SIZE_BITS);	/* L2 offset*/
#else
	bh->b_data = buf;
	buf += BLOCK_SIZE;
	ebh->b_seg = seg;
#endif
    }
}

int INITPROC buffer_init(void)
{
    /* XMS buffers override EXT buffers override internal buffers*/
#if defined(CONFIG_FS_EXTERNAL_BUFFER) || defined(CONFIG_FS_XMS_BUFFER)
    int bufs_to_alloc = CONFIG_FS_NR_EXT_BUFFERS;
    int xms_enabled = 0;

#ifdef CONFIG_FS_XMS_BUFFER
    xms_enabled = xms_init();	/* try to enable unreal mode and A20 gate*/
    if (xms_enabled)
	bufs_to_alloc = CONFIG_FS_NR_XMS_BUFFERS;
#endif
    if (boot_bufs)
	bufs_to_alloc = boot_bufs;
#ifdef CONFIG_FAR_BUFHEADS
    if (bufs_to_alloc > 2500) bufs_to_alloc = 2500; /* max 64K far bufheads @26 bytes*/
#else
    if (bufs_to_alloc > 256) bufs_to_alloc = 256; /* protect against high XMS value*/
#endif

    printk("%d %s buffers, %ld ram\n", bufs_to_alloc, xms_enabled? "xms": "ext",
		(long)bufs_to_alloc << 10);
#else
    int bufs_to_alloc = NR_MAPBUFS;
#endif

    buffer_heads = heap_alloc(bufs_to_alloc * sizeof(struct buffer_head),
	HEAP_TAG_BUFHEAD|HEAP_TAG_CLEAR);
    if (!buffer_heads) return 1;
#ifdef CONFIG_FAR_BUFHEADS
    size_t size = bufs_to_alloc * sizeof(ext_buffer_head);
    segment_s *seg = seg_alloc((size + 15) >> 4, SEG_FLAG_EXTBUF);
    if (!seg) return 1;
    fmemsetw(0, seg->base, 0, size >> 1);
    ext_buffer_heads = _MK_FP(seg->base, 0);
#endif
    bh_next = bh_lru = bh_llru = buffer_heads;

#if defined(CONFIG_FS_EXTERNAL_BUFFER) || defined(CONFIG_FS_XMS_BUFFER)
    do {
	int nbufs;

	/* allocate buffers in 64k chunks so addressable with segment/offset*/
	if ((nbufs = bufs_to_alloc) > 64)
	    nbufs = 64;
	bufs_to_alloc -= nbufs;
#ifdef CONFIG_FS_XMS_BUFFER
	if (xms_enabled) {
	    ramdesc_t seg = xms_alloc((long_t)nbufs << BLOCK_SIZE_BITS);
	    add_buffers(nbufs, 0, seg);
	} else
#endif
	{
	    segment_s *seg = seg_alloc (nbufs << (BLOCK_SIZE_BITS - 4),
		SEG_FLAG_EXTBUF|SEG_FLAG_ALIGN1K);
	    if (!seg) return 2;
	    add_buffers(nbufs, 0, seg->base);
	}
    } while (bufs_to_alloc > 0);
#else
    /* no EXT or XMS buffers, internal L1 only */
    add_buffers(NR_MAPBUFS, (char *)L1buf, kernel_ds);
#endif
    return 0;
}

/*
 *	Wait on a buffer
 */

void wait_on_buffer(struct buffer_head *bh)
{
    ext_buffer_head *ebh = EBH(bh);

    while (ebh->b_locked) {
	INR_COUNT(ebh);
	sleep_on((struct wait_queue *)bh);	/* use bh as wait address*/
	DCR_COUNT(ebh);
    }
}

void lock_buffer(struct buffer_head *bh)
{
    wait_on_buffer(bh);
    EBH(bh)->b_locked = 1;
}

void unlock_buffer(struct buffer_head *bh)
{
    EBH(bh)->b_locked = 0;
    wake_up((struct wait_queue *)bh);	/* use bh as wait address*/
}

void invalidate_buffers(kdev_t dev)
{
    struct buffer_head *bh = bh_llru;
    ext_buffer_head *ebh;

    do {
	ebh = EBH(bh);

	if (ebh->b_dev != dev) continue;
	wait_on_buffer(bh);
	if (ebh->b_count) continue;
	ebh->b_uptodate = 0;
	ebh->b_dirty = 0;
	unlock_buffer(bh);
    } while ((bh = ebh->b_prev_lru) != NULL);
}

static void sync_buffers(kdev_t dev, int wait)
{
    struct buffer_head *bh = bh_llru;
    ext_buffer_head *ebh;

    do {
	ebh = EBH(bh);

	/*
	 *      Skip clean buffers.
	 */
	if ((dev && (ebh->b_dev != dev)) || !ebh->b_dirty)
	   continue;

	/*
	 *      Locked buffers..
	 *
	 *      If buffer is locked; skip it unless wait is requested
	 *      AND pass > 0.
	 */
	if (ebh->b_locked) {
	    if (!wait) continue;
	    wait_on_buffer(bh);
	}

	/*
	 *      Do the stuff
	 */
	INR_COUNT(ebh);
	ll_rw_blk(WRITE, bh);
	DCR_COUNT(ebh);
    } while ((bh = ebh->b_prev_lru) != NULL);
}

static struct buffer_head *get_free_buffer(void)
{
    struct buffer_head *bh = bh_lru;
    ext_buffer_head *ebh = EBH(bh);

    while (ebh->b_count || ebh->b_dirty || ebh->b_locked
#if defined(CONFIG_FS_EXTERNAL_BUFFER) || defined(CONFIG_FS_XMS_BUFFER)
		|| bh->b_data
#endif
								) {
	if ((bh = ebh->b_next_lru) == NULL) {
	    sync_buffers(0, 0);
	    bh = bh_lru;
	}
	ebh = EBH(bh);
    }
    put_last_lru(bh);
    ebh->b_uptodate = 0;
    ebh->b_count = 1;
    SET_COUNT(ebh);
    return bh;
}

/*
 * Release a buffer head
 */

void __brelse(struct buffer_head *bh)
{
    ext_buffer_head *ebh = EBH(bh);

    wait_on_buffer(bh);
    if (ebh->b_count == 0) panic("brelse");
#if 0
    if (!--ebh->b_count)
	wake_up(&bufwait);
#else
    DCR_COUNT(ebh);
#endif
}

void brelse(struct buffer_head *bh)
{
    if (bh) __brelse(bh);
}

/*
 * bforget() is like brelse(), except it removes the buffer
 * data validity.
 */
#if 0
void __bforget(struct buffer_head *bh)
{
    ext_buffer_head *ebh = EBH(bh);

    wait_on_buffer(bh);
    ebh->b_dirty = 0;
    DCR_COUNT(ebh);
    ebh->b_dev = NODEV;
    wake_up(&bufwait);
}
#endif

/* Turns out both minix_bread and bread do this, so I made this a function
 * of it's own... */

struct buffer_head *readbuf(struct buffer_head *bh)
{
    ext_buffer_head *ebh = EBH(bh);

    if (!ebh->b_uptodate) {
	ll_rw_blk(READ, bh);
	wait_on_buffer(bh);
	if (!ebh->b_uptodate) {
	    brelse(bh);
	    bh = NULL;
	}
    }
    return bh;
}

static struct buffer_head *find_buffer(kdev_t dev, block32_t block)
{
    struct buffer_head *bh = bh_llru;
    ext_buffer_head *ebh;

    do {
	ebh = EBH(bh);

	if (ebh->b_blocknr == block && ebh->b_dev == dev) break;
    } while ((bh = ebh->b_prev_lru) != NULL);
    return bh;
}

struct buffer_head *get_hash_table(kdev_t dev, block_t block)
{
    struct buffer_head *bh;

    if ((bh = find_buffer(dev, (block32_t)block)) != NULL) {
	ext_buffer_head *ebh = EBH(bh);

	INR_COUNT(ebh);
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

/* 16 bit block numbers for super blocks and MINIX filesystem driver*/
struct buffer_head *getblk(kdev_t dev, block_t block)
{
	return getblk32(dev, (block32_t)block);
}

/* 32 bit block numbers for FAT filesystem driver*/
struct buffer_head *getblk32(kdev_t dev, block32_t block)
{
    struct buffer_head *bh;
    struct buffer_head *n_bh;
    ext_buffer_head *ebh;

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
 * and that it's unused (b_count=0), unlocked (b_locked=0), and clean
 */
    ebh = EBH(bh);
    ebh->b_dev = dev;
    ebh->b_blocknr = block;
#if defined(CONFIG_FS_EXTERNAL_BUFFER) || defined(CONFIG_FS_XMS_BUFFER)
    ebh->b_seg = bh->b_data? kernel_ds: ebh->b_ds;
#endif
    goto return_it;

  found_it:
    if (n_bh != NULL) {
	ext_buffer_head *en_bh = EBH(n_bh);

	CLR_COUNT(en_bh);
	en_bh->b_count = 0;	/* Release previously created buffer head */
    }
    ebh = EBH(bh);
    INR_COUNT(ebh);
    wait_on_buffer(bh);
    if (!ebh->b_dirty && ebh->b_uptodate)
	put_last_lru(bh);

  return_it:
    return bh;
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It returns NULL if the block was unreadable.
 */

/* 16 bit block numbers for super blocks and MINIX filesystem driver*/
struct buffer_head *bread(kdev_t dev, block_t block)
{
    return readbuf(getblk(dev, block));
}

/* 32 bit block numbers for FAT filesystem driver*/
struct buffer_head *bread32(kdev_t dev, block32_t block)
{
    return readbuf(getblk32(dev, block));
}

#if 0

/* NOTHING is using breada at this point, so I can pull it out... Chad */
struct buffer_head *breada(kdev_t dev, block_t block, int bufsize,
			   unsigned int pos, unsigned int filesize)
{
    struct buffer_head *bh, *bha;
    int i, j;

    if (pos >= filesize)
	return NULL;

    if (block < 0)
	return NULL;
    bh = getblk(dev, block);

    if (bh->b_uptodate)
	return bh;

    bha = getblk(dev, block + 1);
    if (bha->b_uptodate) {
	brelse(bha);
	bha = NULL;
    } else {
	/* Request the read for these buffers, and then release them */
	ll_rw_blk(READ, bha);
	brelse(bha);
    }
    /* Wait for this buffer, and then continue on */
    wait_on_buffer(bh);
    if (bh->b_uptodate)
	return bh;
    brelse(bh);
    return NULL;
}

#endif

void mark_buffer_uptodate(struct buffer_head *bh, int on)
{
    ext_buffer_head *ebh = EBH(bh);
    flag_t flags;

    save_flags(flags);
    clr_irq();
    ebh->b_uptodate = on;
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

#if defined(CONFIG_FS_EXTERNAL_BUFFER) || defined(CONFIG_FS_XMS_BUFFER)
/* map_buffer copies a buffer into L1 buffer space. It will freeze forever
 * before failing, so it can return void.  This is mostly 8086 dependant,
 * although the interface is not.
 */
void map_buffer(struct buffer_head *bh)
{
    ext_buffer_head *ebh = EBH(bh);
    int i;

    /* If buffer is already mapped, just increase the refcount and return */
    if (bh->b_data /*|| ebh->b_seg != kernel_ds*/) {
	if (!ebh->b_mapcount)
	    debug("REMAP: %d\n", buf_num(bh));
	goto end_map_buffer;
    }

    i = lastL1map;
    /* search for free L1 buffer or wait until one is available*/
    for (;;) {
	struct buffer_head *bmap;
	ext_buffer_head *ebmap;

	if (++i >= NR_MAPBUFS) i = 0;
	debug("map:   %d try %d\n", buf_num(bh), i);

	/* First check for the trivial case, to avoid dereferencing a null pointer */
	if (!(bmap = L1map[i]))
	    break;
	ebmap = EBH(bmap);

	/* L1 with zero count can be unmapped and reused for this request*/
	if (ebmap->b_mapcount < 0)
		printk("map_buffer: %d BAD mapcount %d\n", buf_num(bmap), ebmap->b_mapcount);
	if (!ebmap->b_mapcount) {
	    debug("UNMAP: %d <- %d\n", buf_num(bmap), i);

	    /* Unmap/copy L1 to L2 */
	    xms_fmemcpyw(ebmap->b_L2data, ebmap->b_ds, bmap->b_data, kernel_ds, BLOCK_SIZE/2);
	    bmap->b_data = (char *)0;
	    ebmap->b_seg = ebmap->b_ds;
	    break;		/* success */
	}
	if (i == lastL1map) {
	    /* no free L1 buffers, must wait for L1 unmap_buffer*/
	    debug("MAPWAIT: %d\n", buf_num(bh));
	    sleep_on(&L1wait);
	}
    }

    /* Map/copy L2 to L1 */
    lastL1map = i;
    L1map[i] = bh;
    bh->b_data = (char *)L1buf + (i << BLOCK_SIZE_BITS);
    if (ebh->b_uptodate)
	xms_fmemcpyw(bh->b_data, kernel_ds, ebh->b_L2data, ebh->b_ds, BLOCK_SIZE/2);
    debug("MAP:   %d -> %d\n", buf_num(bh), i);
  end_map_buffer:
    ebh->b_seg = kernel_ds;
    ebh->b_mapcount++;
}

/* unmap_buffer decreases bh->b_mapcount, and wakes up anyone waiting over
 * in map_buffer if it's decremented to 0... this is a bit of a misnomer,
 * since the unmapping is actually done in map_buffer to prevent frivoulous
 * unmaps if possible.
 */
void unmap_buffer(struct buffer_head *bh)
{
    if (bh) {
	ext_buffer_head *ebh = EBH(bh);

	if (ebh->b_mapcount <= 0) {
	    printk("unmap_buffer: %d BAD mapcount %d\n", buf_num(bh), ebh->b_mapcount);
	    ebh->b_mapcount = 0;
	} else if (--ebh->b_mapcount == 0) {
	    debug("unmap: %d\n", buf_num(bh));
	    wake_up(&L1wait);
	} else
	    debug("unmap_buffer: %d mapcount %d\n", buf_num(bh), ebh->b_mapcount+1);
    }
}

void unmap_brelse(struct buffer_head *bh)
{
    if (bh) {
	unmap_buffer(bh);
	__brelse(bh);
    }
}

char *buffer_data(struct buffer_head *bh)
{
    return (bh->b_data? bh->b_data: EBH(bh)->b_L2data);
}
#endif /* CONFIG_FS_EXTERNAL_BUFFER | CONFIG_FS_XMS_BUFFER*/
