/*
 *	Memory management support for a swapping rather than paging kernel.
 *
 *	Memory allocator for ELKS. We keep a hole list so we can keep the
 *	malloc arena data in the kernel not scattered into hard to read
 *	user memory.
 *
 *	20th Jan 2000	Alistair Riddoch (ajr@ecs.soton.ac.uk)
 *			For reasons explained in fs/exec.c, support is
 *			required for holes which do not start at the
 *			the bottom of the segment. To facilitate this
 *			I have added a hole_seg field to the hole descriptor
 *			structure which indicates the segment number the hole
 *			exists in. Later we will add support for growing holes
 *			in both directions to allow for a growing stack at the
 *			bottom and a heap at the top.
 */

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
#include <linuxmt/mem.h> /* for mem_swap_info */
#include <linuxmt/debug.h>

/*
 *	Worst case is code+data segments for max tasks unique processes
 *	and one for the free space left.
 */

#define MAX_SEGMENTS		8+(MAX_TASKS*2)
#define MAX_SWAP_SEGMENTS	8+(MAX_TASKS*2)

struct malloc_hole {
    seg_t page_base;		/* Pages */
    segext_t extent;		/* Pages */
    struct malloc_hole *next;	/* Next in list memory order */
    __u8 refcount;
    __u8 flags;			/* So we know if it is free */
#define HOLE_USED		1
#define HOLE_FREE		2
#define HOLE_SPARE		3
#define HOLE_SWAPPED		8
};

static struct malloc_hole holes[MAX_SEGMENTS];

#ifdef CONFIG_SWAP
static struct malloc_hole swap_holes[MAX_SWAP_SEGMENTS];
#endif

struct malloc_head {
    struct malloc_hole *holes;
    int size;
};

struct malloc_head memmap = { holes, MAX_SEGMENTS };

#ifdef CONFIG_SWAP
struct malloc_head swapmap = { swap_holes, MAX_SWAP_SEGMENTS };
static struct buffer_head swap_buf;
static dev_t swap_dev;
#endif

/*
 *	Find a spare hole.
 */

static struct malloc_hole *alloc_hole(struct malloc_head *mh)
{
    register struct malloc_hole *m = mh->holes;
    register char *ct = (char *)(mh->size);

    do {
	if (m->flags == HOLE_SPARE)
	    return m;
	m++;
    } while (--ct);
    panic("mm: too many holes");
}

/*
 *	Split a hole into two
 */

static void split_hole(struct malloc_head *mh,
		       register struct malloc_hole *m, segext_t len)
{
    register struct malloc_hole *n;
    seg_t spare = m->extent - len;

    m->flags = HOLE_USED;
    if (!spare)
	return;
    /*
     *      Split into one allocated one free
     */

    m->extent = len;
    n = alloc_hole(mh);
    n->page_base = m->page_base + len;
    n->extent = spare;
    n->next = m->next;
    n->flags = HOLE_FREE;
    m->next = n;
}

/*
 *	Merge adjacent free holes
 */

static void sweep_holes(struct malloc_head *mh)
{
    register struct malloc_hole *m = mh->holes;

    while (m != NULL && m->next != NULL) {
	if (m->flags == HOLE_FREE && m->next->flags == HOLE_FREE &&
	    m->page_base + m->extent == m->next->page_base) {
	    m->extent += m->next->extent;
	    m->next->flags = HOLE_SPARE;
	    m->next = m->next->next;
	} else
	    m = m->next;
    }
}

#if 0

void dmem(struct malloc_head *mh)
{
    register struct malloc_hole *m;
    char *status;
        if(mh)m = mh->holes;else m = memmap.holes;
    do {
	switch (m->flags) {
	case HOLE_SPARE:
	    status = "SPARE";
	    break;
	case HOLE_FREE:
	    status = "FREE";
	    break;
	case HOLE_USED:
	    status = "USED";
	    break;
	default:
	    status = "DODGY";
	    break;
	}
	printk("HOLE %x size %x next start %x is %s\n", m->page_base, m->extent, m->page_base + m->extent, status);
	m = m->next;
    } while (m);
}

#endif

/*
 *	Find the nearest fitting hole
 */

static struct malloc_hole *best_fit_hole(struct malloc_head *mh, segext_t size)
{
    register struct malloc_hole *m = mh->holes;
    register struct malloc_hole *best = NULL;

    while (m) {
	if (m->flags == HOLE_FREE && m->extent >= size)
	    if (!best || best->extent > m->extent)
		best = m;
	m = m->next;
    }
    return best;
}

/*
 *	Find the hole starting at a given location
 */

static struct malloc_hole *find_hole(struct malloc_head *mh, seg_t base)
{
    register struct malloc_hole *m = mh->holes;

    while (m && (m->page_base != base)) {
	m = m->next;
    }
    return m;
}

/*
 *	Allocate a segment
 */

#ifdef CONFIG_SWAP

int swap_out(seg_t base);
seg_t swap_strategy();
int swap_out_someone();

#endif

seg_t mm_alloc(segext_t pages)
{
    /*
     *      Which hole fits best ?
     */
    register struct malloc_hole *m;

#ifdef CONFIG_SWAP

    while ((m = best_fit_hole(&memmap, pages)) == NULL) {
	seg_t s = swap_strategy(NULL);
	if (s == NULL || swap_out(s) == -1)
	    return NULL;
    }

#else

    m = best_fit_hole(&memmap, pages);
    if (m == NULL)
	return NULL;

#endif

    /*
     *      The hole is (probably) too big
     */

    split_hole(&memmap, m, pages);
    m->refcount = 1;

    return m->page_base;
}

/*
 * This function will swapin the whole process. After
 * there are exact ideas on the implementation of the swap cache
 * this sould change
 */

#ifdef CONFIG_SWAP

static seg_t validate_address(seg_t base)
{
    register struct task_struct *t;

    for_each_task(t) {
	if (t->mm.cseg == base && t->mm.flags & CS_SWAP) {
	    do_swapper_run(t);
	    return t->mm.cseg;
	}
	if (t->mm.dseg == base && t->mm.flags & DS_SWAP) {
	    do_swapper_run(t);
	    return t->mm.dseg;
	}
    }
    return base;
}

#endif

/* 	Increase refcount */
seg_t mm_realloc(seg_t base)
{
    register struct malloc_hole *m;

#ifdef CONFIG_SWAP

    base = validate_address(base);

#endif

    m = find_hole(&memmap, base);
    m->refcount++;

    return m->page_base;
}

/*
 *	Free a segment.
 *
 *	Caution: A process can be killed dead by another. That means we
 *	might potentially have to kill a swapped task. Be sure to catch
 *	that in exit...
 */

void mm_free(seg_t base)
{
    register struct malloc_head *mh = &memmap;
    register struct malloc_hole *m;

    m = find_hole(mh, base);
    if (!m) {

#ifdef CONFIG_SWAP

	m = find_hole(&swapmap, base);
	if (!m)
	    panic("mm corruption");
	mh = &swapmap;
	printk("mm_free(): from swap\n");

#else

	panic("mm corruption");

#endif

    }

    if (m->flags != HOLE_USED)
	panic("double free");
    if (!(--(m->refcount))) {
	m->flags = HOLE_FREE;
	sweep_holes(mh);
    }
}

/*
 *	Allocate a segment and copy it from another
 */

seg_t mm_dup(seg_t base)
{
    register struct malloc_hole *o, *m;

    debug("MALLOC: mm_dup()\n");
    o = find_hole(&memmap, base);
    if (o->flags != HOLE_USED)
	panic("bad/swapped hole");

#ifdef CONFIG_SWAP

    while ((m = best_fit_hole(&memmap, o->extent)) == NULL) {
	seg_t s = swap_strategy(NULL);
	if (!s || swap_out(s) == -1)
	    return NULL;
    }

#else

    m = best_fit_hole(&memmap, o->extent);
    if (m == NULL)
	return NULL;

#endif

    split_hole(&memmap, m, o->extent);
    m->refcount = 1;
    fmemcpy(m->page_base, 0, o->page_base, 0, (__u16)(o->extent << 4));
    return m->page_base;
}

/*
 * Returns memory usage information in KB's.
 * "type" is either MM_MEM or MM_SWAP and "used"
 * selects if we request the used or free memory.
 */
unsigned int mm_get_usage(int type, int used)
{
    register struct malloc_hole *m;
    int flag;
    unsigned int ret = 0;

#ifdef CONFIG_SWAP

    m = type == MM_MEM ? memmap.holes : swapmap.holes;

#else

    m = memmap.holes;

#endif

    flag = used ? HOLE_USED : HOLE_FREE;

    while (m != NULL) {
	if ((int) m->flags == flag)
	    ret += m->extent;
	m = m->next;
    }

#ifdef CONFIG_SWAP

    if (type != MM_MEM)
	return ret;

#endif

    return ret >> 6;
}

/*
 * Resize a hole
 */
struct malloc_hole *mm_resize(register struct malloc_hole *m, segext_t pages)
{
    register struct malloc_hole *next;
    segext_t ext;
    seg_t base;
    if(m->extent >= pages){
        /* for now don't reduce holes */
        return m;
    }

    next = m->next;
    ext = pages - m->extent;
    if(next->flags == HOLE_FREE && next->extent >= ext){
        m->extent += ext;
        next->extent -= ext;
        next->page_base += ext;
        if(next->extent == 0){
            next->flags == HOLE_SPARE;
            m->next = next->next;
        }
        return m;
    }

#ifdef CONFIG_ADVANCED_MM

    base = mm_alloc(pages);
    if(!next){
        return NULL; /* Out of luck */
    }
    fmemcpy(base, 0, m->page_base, 0, (__u16)(m->extent << 4));
    next = find_hole(&memmap, base);
    next->refcount = m->refcount;
    m->flags = HOLE_FREE;
    sweep_holes(&memmap);

    return next;

#else

    return NULL;

#endif

}

int sys_brk(__pptr len)
{
    register __ptask currentp = current;
#ifdef CONFIG_EXEC_ELKS
    register struct malloc_hole *h;
#endif

#if 0
    printk("sbrk: l %d, endd %x, bstack %x, endbrk %x, endseg %x, mem %x/%x\n",
		    len, currentp->t_enddata, currentp->t_begstack,
		    currentp->t_endbrk, currentp->t_endseg,
		    mm_get_usage(MM_MEM, 1),
		    mm_get_usage(MM_MEM, 0));
#endif

    if (len < currentp->t_enddata)
        return -ENOMEM;
    if (currentp->t_begstack > currentp->t_endbrk)
        if(len > currentp->t_endseg - 0x1000) {
		printk("sys_brk failed: len %u > endseg %u\n", len, (currentp->t_endseg - 0x1000));
            return -ENOMEM;
	}

#ifdef CONFIG_EXEC_ELKS
    if(len > currentp->t_endseg){
        /* Resize time */

        h = find_hole(&memmap, currentp->mm.dseg);

        h = mm_resize(h, (len + 15) >> 4);
        if(!h){
            return -ENOMEM;
        }
        if(h->refcount != 1){
            panic("Relocated shared hole");
        }

        currentp->mm.dseg = h->page_base;
        currentp->t_regs.ds = h->page_base;
        currentp->t_regs.ss = h->page_base;
        currentp->t_endseg = len;
    }
#endif
brk_return:
    currentp->t_endbrk = len;

    return 0;
}

/*
 *	Initialise the memory manager.
 */

void mm_init(seg_t start, seg_t end)
{
    register struct malloc_hole *holep = &holes[MAX_SEGMENTS - 1];

    /*
     *      Mark pages free.
     */
    do {
	holep->flags = HOLE_SPARE;
    } while(--holep > holes);

    /*
     *      Single hole containing all user memory.
     */
    holep->flags = HOLE_FREE;
    holep->page_base = start;
    holep->extent = end - start;
    holep->refcount = 0;
    holep->next = NULL;

#ifdef CONFIG_SWAP
    swap_dev = NULL;
    mm_swap_on(NULL);
#endif

}

#ifdef CONFIG_SWAP
/*
 *	Swapper task
 */

/*
 *	Push a segment to disk if possible
 */

int mm_swap_on(struct mem_swap_info *si)
{
    register struct malloc_hole *holep;
    int ct;
    if(swap_dev)
    	return -EPERM;

    for (ct = 1; ct < MAX_SWAP_SEGMENTS; ct++)
	swap_holes[ct].flags = HOLE_SPARE;

    if(!si)
    	return 0;

    holep = &swap_holes[0];
    holep->flags = HOLE_FREE;
    holep->page_base = 0;
    holep->extent = si->size;
    holep->refcount = 0;
    holep->next = NULL;

    swap_dev = (si->major << 8) + si->minor;

    return 0;
}

static int swap_out(seg_t base)
{
    register struct task_struct *t;
    register struct malloc_hole *o = find_hole(&memmap, base);
    struct malloc_hole *so;
    int ct, blocks;

    /* We can hit disk this time. Allocate a hole in 1K increments */
    blocks = (o->extent + 0x3F) >> 6;
    so = best_fit_hole(&swapmap, blocks);
    if (so == NULL) {
	/* No free swap */
	return -1;
    }
    split_hole(&swapmap, so, blocks);
    so->refcount = o->refcount;

    for_each_task(t) {
	int c = t->mm.flags;
	if (t->mm.cseg == base && !(c & CS_SWAP)) {
	    t->mm.cseg = so->page_base;
	    t->mm.flags |= CS_SWAP;
	    debug2("MALLOC: swaping out code of pid %d blocks %d\n",
		   t->pid, blocks);
	}
	if (t->mm.dseg == base && !(c & DS_SWAP)) {
	    t->mm.dseg = so->page_base;
	    t->mm.flags |= DS_SWAP;
	    debug2("MALLOC: swaping out data of pid %d blocks %d\n",
		   t->pid, blocks);
	}
    }

    /* Now write the segment out */
    for (ct = 0; ct < blocks; ct++) {
	swap_buf.b_blocknr = so->page_base + ct;
	swap_buf.b_dev = swap_dev;
	swap_buf.b_lock = 0;
	swap_buf.b_dirty = 1;
	swap_buf.b_seg = o->page_base;
	swap_buf.b_data = ct << 10;
	ll_rw_blk(WRITE, &swap_buf);
	wait_on_buffer(&swap_buf);
    }
    o->flags = HOLE_FREE;
    sweep_holes(&memmap);

    return 1;
}

static int swap_in(seg_t base, int chint)
{
    register struct malloc_hole *o;
    struct malloc_hole *so;
    int ct, blocks;
    register struct task_struct *t;

    so = find_hole(&swapmap, base);
    /* Find memory for this segment */
    o = best_fit_hole(&memmap, so->extent << 6);
    if (o == NULL)
	return -1;

    /* Now read the segment in */
    split_hole(&memmap, o, so->extent << 6);
    o->refcount = so->refcount;

    blocks = so->extent;

    for (ct = 0; ct < blocks; ct++) {
	swap_buf.b_blocknr = so->page_base + ct;
	swap_buf.b_dev = swap_dev;
	swap_buf.b_lock = 0;
	swap_buf.b_dirty = 0;
	swap_buf.b_uptodate = 0;
	swap_buf.b_seg = o->page_base;
	swap_buf.b_data = ct << 10;

	ll_rw_blk(READ, &swap_buf);
	wait_on_buffer(&swap_buf);
    }

    /*
     *      Update the memory management tables
     */
    for_each_task(t) {
	int c = t->mm.flags;
	if (t->mm.cseg == base && c & CS_SWAP) {
	    debug2("MALLOC: swapping in code of pid %d seg %x\n",
		   t->pid, t->mm.cseg);
	    t->mm.cseg = o->page_base;
	    t->mm.flags &= ~CS_SWAP;
	}
	if (t->mm.dseg == base && c & DS_SWAP) {
	    debug2("MALLOC: swapping in data of pid %d seg %x\n",
		   t->pid, t->mm.dseg);
	    t->mm.dseg = o->page_base;
	    t->mm.flags &= ~DS_SWAP;
	}
	if (c && !t->mm.flags) {
	    t->t_regs.cs = t->mm.cseg;
	    t->t_regs.ds = t->mm.dseg;
	    t->t_regs.ss = t->mm.dseg;

	    put_ustack(t, 2, t->t_regs.cs);
	}
    }

    /* Our equivalent of the Linux swap cache. Try and avoid writing CS
     * back. Need to kill segments on last exit for this to work, and
     * keep a table - TODO
     */
#if 0
    if (chint==0)
#endif
    {
	so->refcount = 0;
	so->flags = HOLE_FREE;
	sweep_holes(&swapmap);
    }

    return 0;
}

/*
 *	When the swapper has found a task it wishes to make run we do the
 *	dirty work. On failure we return -1
 */
static int make_runnable(register struct task_struct *t)
{
    char flags = t->mm.flags;

    if (flags & CS_SWAP)
	if (swap_in(t->mm.cseg, 1) == -1)
	    return -1;
    if (flags & DS_SWAP)
	if (swap_in(t->mm.dseg, 0) == -1)
	    return -1;
    return 0;
}

static seg_t swap_strategy(register struct task_struct *swapin_target)
{
    register struct task_struct *t;
    struct malloc_hole *o;
    seg_t best_ret = 0;
    int ret, rate;
    int best_rate = -1;
    int best_pid;

    debug1("swap_strategy(pid %d)\n", swapin_target->pid);
    for_each_task(t) {
	if (   t->state == TASK_UNUSED
	    || t->pid == 0
	    || t->mm.cseg == current->mm.cseg
	    )
	    continue;
	if(swapin_target != NULL && t->mm.cseg == swapin_target->mm.cseg)
	    continue;

	ret = 0;
	rate = 0;
	if (t->state != TASK_RUNNING)
	    rate++;

	rate += (jiffies - t->last_running) >> 4;

	if (!(t->mm.flags & CS_SWAP)) {
	    if (t->mm.flags & DS_SWAP)
		rate += 1000;
	    ret = t->mm.cseg;
	}

	if (!(t->mm.flags & DS_SWAP) && ret == 0) {
	    if (t->mm.flags & CS_SWAP)
		rate += 1000;
	    ret = t->mm.dseg;
	}

	if (ret != 0 && rate > best_rate) {
	    best_rate = rate;
	    best_ret = ret;
	    best_pid = t->pid;
	}
    }

    debug2("Choose pid %d rate %d\n", best_pid, best_rate);

    return best_ret;
}

/*
 *	Do an iteration of the swapper
 */
int do_swapper_run(register struct task_struct *swapin_target)
{
    while (make_runnable(swapin_target) == -1) {
	seg_t t = swap_strategy(swapin_target);
	if (!t || swap_out(t) == -1)
	    return -1;
    }
    return 0;
}

#endif
