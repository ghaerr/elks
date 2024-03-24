#include <linuxmt/config.h>
#include <linuxmt/limits.h>
#include <linuxmt/init.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/string.h>
#include <linuxmt/mm.h>
#include <linuxmt/heap.h>
#include <linuxmt/errno.h>
#include <linuxmt/trace.h>
#include <linuxmt/debug.h>

#include <arch/system.h>
#include <arch/segment.h>
#include <arch/io.h>
#include <arch/irq.h>

/*
 * Kernel buffer management.
 *
 * Aug 2023 Greg Haerr - Added dynamic L1 buffers and release L1 mappings during sync.
 */

/* Number of internal L1 buffers, used to map/copy external L2 buffers
   to/from kernel data segment. */
int nr_map_bufs = NR_MAPBUFS;                   /* override with /bootopts cache= */
#define MAX_NR_MAPBUFS  20

#ifdef CONFIG_FS_EXTERNAL_BUFFER
int nr_ext_bufs = CONFIG_FS_NR_EXT_BUFFERS;     /* override with /bootopts buf= */
#endif
#ifdef CONFIG_FS_XMS_BUFFER
int nr_xms_bufs = CONFIG_FS_NR_XMS_BUFFERS;     /* override with /bootopts xmsbuf= */
#endif

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
unsigned char buffer_count(struct buffer_head *bh) { return EBH(bh)->b_count; }
block32_t buffer_blocknr(struct buffer_head *bh)   { return EBH(bh)->b_blocknr; }
kdev_t buffer_dev(struct buffer_head *bh)          { return EBH(bh)->b_dev; }

#endif /* CONFIG_FAR_BUFHEADS */

/* Internal L1 buffers, allocated from kernel near heap, must be DS addressable */
static char *L1buf;

/* Buffer cache */
static struct buffer_head *bh_lru;      /* least recently used - reused/flushed first */
static struct buffer_head *bh_llru;     /* most recently used - for finding a buffer */
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
static struct buffer_head *L1map[MAX_NR_MAPBUFS]; /* L1 indexed pointer to L2 buffer */
static struct wait_queue L1wait;                  /* Wait for a free L1 buffer area */
static int lastL1map;
#endif
static int xms_enabled;
static int map_count, remap_count, unmap_count;

static int nr_free_bh, nr_bh;
#ifdef CHECK_FREECNTS
#define DCR_COUNT(bh) if(!(--bh->b_count))nr_free_bh++
#define INR_COUNT(bh) if(!(bh->b_count++))nr_free_bh--
#define CLR_COUNT(bh) if(bh->b_count)nr_free_bh++
#define SET_COUNT(bh) if(--nr_free_bh < 0) { panic("get_free_buffer: bad free count"); }
#else
#define DCR_COUNT(bh) (bh->b_count--)
#define INR_COUNT(bh) (bh->b_count++)
#define CLR_COUNT(bh)
#define SET_COUNT(bh)
#endif

#define buf_num(bh)     ((bh) - buffer_heads)   /* buffer number, for debugging */

static void put_last_lru(struct buffer_head *bh)
{
    ext_buffer_head *ebh = EBH(bh);

    if (bh_llru != bh) {
        struct buffer_head *bhn = ebh->b_next_lru;
        ext_buffer_head *ebhn = EBH(bhn);

        if ((ebhn->b_prev_lru = ebh->b_prev_lru))       /* Unhook */
            EBH(ebh->b_prev_lru)->b_next_lru = bhn;
        else                                    /* Alter head */
            bh_lru = bhn;
        /*
         *      Put on lru end
         */
        ebh->b_next_lru = NULL;
        EBH(ebh->b_prev_lru = bh_llru)->b_next_lru = bh;
        bh_llru = bh;
    }
}

static void INITPROC add_buffers(int nbufs, char *buf, ramdesc_t seg)
{
    struct buffer_head *bh;
    int n = 0;
    size_t offset;

    for (bh = bh_next; n < nbufs; n++, bh = ++bh_next) {
        ext_buffer_head *ebh = EBH(bh);

        if (bh != buffer_heads) {
            ebh->b_next_lru = ebh->b_prev_lru = bh;
            put_last_lru(bh);
        }

#if defined(CONFIG_FS_EXTERNAL_BUFFER) || defined(CONFIG_FS_XMS_BUFFER)
        /* segment adjusted to require no offset to buffer */
        offset = xms_enabled? ((n & 63) << BLOCK_SIZE_BITS) :
                              ((n & 63) << (BLOCK_SIZE_BITS - 4));
        ebh->b_L2seg = seg + offset;
#else
        bh->b_data = buf;
        buf += BLOCK_SIZE;
#endif
    }
}

#if defined(CHECK_FREECNTS) && DEBUG_EVENT
static void list_buffer_status(void)
{
    int i = 1;
    int inuse = 0;
    int isinuse, j;
    struct buffer_head *bh = bh_llru;
    ext_buffer_head *ebh;

    do {
        ebh = EBH(bh);
        isinuse = ebh->b_count || ebh->b_dirty || ebh->b_locked || ebh->b_mapcount;
        if (isinuse || bh->b_data) {
            j = 0;
            if (bh->b_data) {
                for (; j<nr_map_bufs; j++) {
                    if (L1map[j] == bh) {
                        j++;
                        break;
                    }
                }
            }
            printk("\n#%3d: buf %3d blk/dev %5ld/%p %c%c%c %smapped L%02d %d count %d",
                i, buf_num(bh), ebh->b_blocknr, ebh->b_dev,
                ebh->b_locked?  'L': ' ',
                ebh->b_dirty?   'D': ' ',
                ebh->b_uptodate?'U': ' ',
                j? "  ": "un", j, ebh->b_mapcount, ebh->b_count);
        }
        i++;
        if (isinuse) inuse++;
    } while ((bh = ebh->b_prev_lru) != NULL);
    printk("\nTotal L2 buffers inuse %d/%d (%d free)", inuse, nr_bh, nr_free_bh);
    printk(", %dk L1 (map %u, unmap %u remap %u)\n",
        nr_map_bufs, map_count, unmap_count, remap_count);
}
#endif

int INITPROC buffer_init(void)
{
    if (nr_map_bufs > MAX_NR_MAPBUFS) nr_map_bufs = MAX_NR_MAPBUFS;

    /* XMS buffers override EXT buffers override internal buffers*/
#if defined(CONFIG_FS_EXTERNAL_BUFFER) || defined(CONFIG_FS_XMS_BUFFER)
    int bufs_to_alloc = nr_ext_bufs;

#ifdef CONFIG_FS_XMS_BUFFER
    if (nr_xms_bufs)
        xms_enabled = xms_init();       /* try to enable unreal mode and A20 gate*/
    if (xms_enabled)
        bufs_to_alloc = nr_xms_bufs;
#endif
#ifdef CONFIG_FAR_BUFHEADS
    if (bufs_to_alloc > 2975) bufs_to_alloc = 2975; /* max 64K far bufheads @22 bytes*/
#else
    if (bufs_to_alloc > 256) bufs_to_alloc = 256; /* protect against high XMS value*/
#endif

    printk("%d %s buffers (%dK ram), %dK cache, %d req hdrs\n", bufs_to_alloc,
        xms_enabled? "xms": "ext", bufs_to_alloc, nr_map_bufs, NR_REQUEST);
#else
    int bufs_to_alloc = nr_map_bufs;
#endif

    nr_bh = nr_free_bh = bufs_to_alloc;
#if defined(CHECK_FREECNTS) && DEBUG_EVENT
    debug_setcallback(1, list_buffer_status);   /* ^O will generate buffer list */
#endif

    if (!(L1buf = heap_alloc(nr_map_bufs * BLOCK_SIZE, HEAP_TAG_BUFHEAD|HEAP_TAG_CLEAR)))
        return 1;

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
            ramdesc_t xmsseg = xms_alloc((long_t)nbufs << BLOCK_SIZE_BITS);
            add_buffers(nbufs, 0, xmsseg);
        } else
#endif
        {
            segment_s *extseg = seg_alloc (nbufs << (BLOCK_SIZE_BITS - 4),
                SEG_FLAG_EXTBUF|SEG_FLAG_ALIGN1K);
            if (!extseg) return 2;
            add_buffers(nbufs, 0, extseg->base);
        }
    } while (bufs_to_alloc > 0);
#else
    /* no EXT or XMS buffers, internal L1 only */
    add_buffers(nr_map_bufs, L1buf, kernel_ds);
#endif
    return 0;
}

/*
 *      Wait on a buffer
 */

void wait_on_buffer(struct buffer_head *bh)
{
#ifdef CONFIG_ASYNCIO
    ext_buffer_head *ebh = EBH(bh);
    ebh->b_count++;
    wait_set((struct wait_queue *)bh);       /* use bh as wait address */
    for (;;) {
        current->state = TASK_UNINTERRUPTIBLE;
        if (!ebh->b_locked)
            break;
        schedule();
    }
    wait_clear((struct wait_queue *)bh);
    current->state = TASK_RUNNING;
    ebh->b_count--;
#endif
#ifdef CHECK_BLOCKIO
    if (EBH(bh)->b_locked) panic("wait_on_buffer");
#endif
}

void lock_buffer(struct buffer_head *bh)
{
    wait_on_buffer(bh);
    EBH(bh)->b_locked = 1;
}

void unlock_buffer(struct buffer_head *bh)
{
    EBH(bh)->b_locked = 0;
#ifdef CONFIG_ASYNCIO
    wake_up((struct wait_queue *)bh);   /* use bh as wait address*/
#endif
}

void invalidate_buffers(kdev_t dev)
{
    struct buffer_head *bh = bh_llru;
    ext_buffer_head *ebh;

    debug_blk("INVALIDATE dev %x\n", dev);
    do {
        ebh = EBH(bh);

        if (ebh->b_dev != dev) continue;
        wait_on_buffer(bh);
        if (ebh->b_count) {
            printk("invalidate_buffers: skipping active block %ld\n", ebh->b_blocknr);
            continue;
        }
        debug_blk("invalidating blk %ld\n", ebh->b_blocknr);
        ebh->b_uptodate = 0;
        ebh->b_dirty = 0;
        brelseL1(bh, 0);        /* release buffer from L1 if present */
        unlock_buffer(bh);
    } while ((bh = ebh->b_prev_lru) != NULL);
}

static void sync_buffers(kdev_t dev, int wait)
{
    struct buffer_head *bh = bh_lru;
    ext_buffer_head *ebh;
    int count = 0;

    debug_blk("sync_buffers dev %p wait %d\n", dev, wait);
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
            debug_blk("SYNC: dev %p buf %d block %ld LOCKED mapped %d skipped %d data %04x\n",
                ebh->b_dev, buf_num(bh), ebh->b_blocknr, ebh->b_mapcount, !wait,
                bh->b_data);
            if (!wait) continue;
            wait_on_buffer(bh);
        }

        /*
         *      Do the stuff
         */
        debug_blk("sync: dev %p write buf %d block %ld count %d dirty %d\n",
            ebh->b_dev, buf_num(bh), ebh->b_blocknr, ebh->b_count, ebh->b_dirty);
        ebh->b_count++;
        ll_rw_blk(WRITE, bh);
        ebh->b_count--;
        count++;
    } while ((bh = ebh->b_next_lru) != NULL);
    debug_blk("SYNC_BUFFERS END %d wrote %d\n", wait, count);
}

static struct buffer_head *get_free_buffer(void)
{
    struct buffer_head *bh = bh_lru;
    ext_buffer_head *ebh = EBH(bh);
    int i;
    int pass = 0;

    while (ebh->b_count || ebh->b_dirty || ebh->b_locked
#if defined(CONFIG_FS_EXTERNAL_BUFFER) || defined(CONFIG_FS_XMS_BUFFER)
           || bh->b_data
#endif
                                                        ) {
        if ((bh = ebh->b_next_lru) == NULL) {
            sync_buffers(0, 0);
            if (++pass > 1) {
                for (i=0; i<nr_map_bufs; i++) {
                    brelseL1_index(i, 1);   /* release if not mapcount or locked */
                }
            }
            bh = bh_lru;
        }
        ebh = EBH(bh);
    }
#ifdef CHECK_BLOCKIO
    if (ebh->b_mapcount) panic("get_free_buffer"); /* mapped buffer reallocated */
#endif
    put_last_lru(bh);
    ebh->b_uptodate = 0;
    ebh->b_count = 1;
    SET_COUNT(ebh);
    return bh;
}

/*
 * Release a buffer head
 */

void brelse(struct buffer_head *bh)
{
    ext_buffer_head *ebh;

    if (!bh) return;
    wait_on_buffer(bh);
    ebh = EBH(bh);
#ifdef CHECK_BLOCKIO
    if (ebh->b_count == 0) panic("brelse");
#endif
    DCR_COUNT(ebh);
}

#if UNUSED
/*
 * bforget() is like brelse(), except it removes the buffer
 * data validity.
 */
void bforget(struct buffer_head *bh)
{
    ext_buffer_head *ebh;

    if (!bh) return;
    wait_on_buffer(bh);
    ebh = EBH(bh);
    ebh->b_dirty = 0;
    DCR_COUNT(ebh);
    ebh->b_dev = NODEV;
}
#endif

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
        n_bh = get_free_buffer();       /* This function may sleep and someone else */
      start:                            /* can create the block */
        if ((bh = find_buffer(dev, block)) != NULL) goto found_it;
    } while (n_bh == NULL);
    bh = n_bh;                          /* Block not found, use the new buffer */
/* OK, FINALLY we know that this buffer is the only one of its kind,
 * and that it's unused (b_count=0), unlocked (b_locked=0), and clean
 */
    ebh = EBH(bh);
    ebh->b_dev = dev;
    ebh->b_blocknr = block;
    goto return_it;

  found_it:
    if (n_bh != NULL) {
        ext_buffer_head *en_bh = EBH(n_bh);

        CLR_COUNT(en_bh);
        en_bh->b_count = 0;     /* Release previously created buffer head */
    }
    ebh = EBH(bh);
    INR_COUNT(ebh);
    wait_on_buffer(bh);
    if (!ebh->b_dirty && ebh->b_uptodate)
        put_last_lru(bh);

  return_it:
    return bh;
}

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

#if UNUSED
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
    debug_sup("fsync\n");
    sync_buffers(dev, 0);
    sync_supers(dev);
    sync_inodes(dev);
    sync_buffers(dev, 1);
}

void sync_dev(kdev_t dev)
{
    debug_sup("sync\n");
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

/* clear a buffer area to zeros, used to avoid slow map to L1 if possible */
void zero_buffer(struct buffer_head *bh, size_t offset, int count)
{
#if defined(CONFIG_FS_XMS_INT15) || (!defined(CONFIG_FS_EXTERNAL_BUFFER) && !defined(CONFIG_FS_XMS_BUFFER))
#define FORCEMAP 1
#else
#define FORCEMAP 0
#endif
    /* xms int15 doesn't support a memset function, so map into L1 */
    if (FORCEMAP || bh->b_data) {
        map_buffer(bh);
        memset(bh->b_data + offset, 0, count);
        unmap_buffer(bh);
    }
#if !FORCEMAP
    else {
        ext_buffer_head *ebh = EBH(bh);
        xms_fmemset((char *)offset, ebh->b_L2seg, 0, count);
    }
#endif
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

    /* wait for any I/O complete before mapping to prevent bh/req buffer unpairing */
    wait_on_buffer(bh);

    /* If buffer is already mapped, just increase the refcount and return */
    if (bh->b_data) {
#if DEBUG_MAP
        if (!ebh->b_mapcount) {
            for (i=0; i<nr_map_bufs; i++) {
                if (bh == L1map[i])
                    break;
            }
            debug_map("REMAP: L%02d block %ld\n", i+1, ebh->b_blocknr);
        }
#endif
        remap_count++;
        goto end_map_buffer;
    }

    i = lastL1map;
    /* search for free L1 buffer or wait until one is available*/
    for (;;) {
        struct buffer_head *bmap;
        ext_buffer_head *ebmap;

        if (++i >= nr_map_bufs) i = 0;
        debug("map:   %d try %d\n", buf_num(bh), i);

        /* First check for the trivial case, to avoid dereferencing a null pointer */
        if (!(bmap = L1map[i]))
            break;
        ebmap = EBH(bmap);

        /* L1 with zero count can be unmapped and reused for this request*/
#ifdef CHECK_BLOCKIO
        if (ebmap->b_mapcount < 0)
            printk("map_buffer: %d BAD mapcount %d\n", buf_num(bmap), ebmap->b_mapcount);
#endif
        /* don't remap if I/O in progress to prevent bh/req buffer unpairing */
        if (!ebmap->b_mapcount && !ebmap->b_locked) {
            debug_map("UNMAP: L%02d block %ld\n", i+1, ebmap->b_blocknr);
            brelseL1_index(i, 1);       /* Unmap/copy L1 to L2 */
            break;
        }
        if (i == lastL1map) {
            /* no free L1 buffers, must wait for L1 unmap_buffer*/
            debug_map("MAPWAIT: block %ld\n", ebh->b_blocknr);
            sleep_on(&L1wait);
        }
    }

    /* Map/copy L2 to L1 */
    lastL1map = i;
    L1map[i] = bh;
    bh->b_data = L1buf + (i << BLOCK_SIZE_BITS);
    if (ebh->b_uptodate)
        xms_fmemcpyw(bh->b_data, kernel_ds, 0, ebh->b_L2seg, BLOCK_SIZE/2);
    map_count++;
    debug_map("MAP:   L%02d block %ld\n", i+1, ebh->b_blocknr);
  end_map_buffer:
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

#ifdef CHECK_BLOCKIO
        if (ebh->b_mapcount <= 0) {
            printk("unmap_buffer: %d BAD mapcount %d\n", buf_num(bh), ebh->b_mapcount);
            ebh->b_mapcount = 0;
        } else
#endif
        if (--ebh->b_mapcount == 0) {
            debug("unmap: %d\n", buf_num(bh));
            wake_up(&L1wait);
        } else
            debug("unmap_buffer: %d mapcount %d\n", buf_num(bh), ebh->b_mapcount+1);
    }
}

void unmap_brelse(struct buffer_head *bh)
{
    unmap_buffer(bh);
    brelse(bh);
}

/* release L1 buffer by index with optional copyout */
void brelseL1_index(int i, int copyout)
{
    struct buffer_head *bh = L1map[i];
    ext_buffer_head *ebh;

    if (!bh) return;
    ebh = EBH(bh);
    if (ebh->b_mapcount || ebh->b_locked)
        return;
    if (copyout && ebh->b_uptodate && bh->b_data) {
        xms_fmemcpyw(0, ebh->b_L2seg, bh->b_data, kernel_ds, BLOCK_SIZE/2);
        unmap_count++;
    }
    bh->b_data = 0;
    L1map[i] = 0;
}

/* release L1 buffer by buffer head with optional copyout */
void brelseL1(struct buffer_head *bh, int copyout)
{
    int i;

    if (!bh) return;
    for (i = 0; i < nr_map_bufs; i++) {
        if (L1map[i] == bh) {
            brelseL1_index(i, copyout);
            break;
        }
    }
}

ramdesc_t buffer_seg(struct buffer_head *bh)
{
    return (bh->b_data? kernel_ds: EBH(bh)->b_L2seg);
}

char *buffer_data(struct buffer_head *bh)
{
    return (bh->b_data? bh->b_data: 0); /* L2 addresses are at offset 0 */
}
#endif /* CONFIG_FS_EXTERNAL_BUFFER | CONFIG_FS_XMS_BUFFER*/
