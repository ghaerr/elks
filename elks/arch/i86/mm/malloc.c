/*
 *	Memory management support.
 *
 *	Two possible memory allocators:
 *
 *	- table-based: manage the segments in a single table located in the kernel
 *	  data. Minimize the number of segments to keep the protected mode easy.
 *
 *	- list-based: manage the segment in a list scattered over the whole memory.
 *	  Number of segments is only limited by the available memory but no more
 *	  compatible with the protected mode.
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
//#include <linuxmt/mem.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>
#include <asm/seglist.h>

#define MIN_STACK_SIZE 0x1000	/* 4k min stack above heap*/

#ifdef CONFIG_MEM_TABLE

/*
 *	Worst case is code+data segments for max tasks unique processes
 *	and one for the free space left.
 */

#define MAX_SEGMENTS		8+(MAX_TASKS*2)

struct malloc_hole {
    seg_t page_base;		/* Pages */
    segext_t extent;		/* Pages */
    struct malloc_hole *next;	/* Next in list memory order */
    __u8 refcount;		/* Only meaningful if HOLE_USED */
    __u8 flags;			/* So we know if it is free */
#define HOLE_USED		1
#define HOLE_FREE		2
#define HOLE_SPARE		3
};

static struct malloc_hole holes[MAX_SEGMENTS];

struct malloc_head {
    struct malloc_hole *holes;
    int size;
};

struct malloc_head memmap = { holes, MAX_SEGMENTS };

#endif /* CONFIG_MEM_TABLE */


#ifdef CONFIG_MEM_LIST

#define SEG_FLAG_USED 0x0001

/* Segment header */
/* No more than one paragraph */

#define SEG_HEAD_NODE  0
#define SEG_HEAD_SIZE  4
#define SEG_HEAD_FLAGS 6
#define SEG_HEAD_COUNT 8

struct seg_head_s {
	seglist_node_t node;
	seg_t          size;
	word_t         flags;
	word_t         ref_count;
};

typedef struct seg_head_s seg_head_t;

static seg_t _seg_first;

#endif /* CONFIG_MEM_LIST */


#ifdef CONFIG_MEM_TABLE

/*
 *	Split a hole into two
 */

static void split_hole(struct malloc_head *mh,
		       register struct malloc_hole *m, segext_t len)
{
    register struct malloc_hole *n;
    int ct;

    m->flags = HOLE_USED;
    if (m->extent > len) {
    /*
     *	Find a spare hole.
     */
	ct = mh->size;
	n = mh->holes;
	while (n->flags != HOLE_SPARE) {
	    n++;
	    if (!--ct)		/* If no spare holes */
		return;		/* Try to continue by no splitting the hole */
	}

    /*
     *      Split into one allocated one free
     */

	n->flags = HOLE_FREE;
	n->page_base = m->page_base + len;
	n->next = m->next;
	n->extent = m->extent - len;
	m->extent = len;
	m->next = n;
    }
}

/*
 *	Merge adjacent free holes
 */

static void free_hole(register struct malloc_hole *n, register struct malloc_hole *m)
{
    m->flags = HOLE_FREE;
    if (m != n) {
	while (n->next != m)		/* Find the hole before hole m */
	    n = n->next;
	if (n->flags != HOLE_FREE)	/* Will merge up to 3 holes */
	    n = m;			/* starting with hole n */
    }
    while ((m = n->next) != NULL && m->flags == HOLE_FREE) {
	n->extent += m->extent;		/* Merge hole n with its next hole */
	m->flags = HOLE_SPARE;
	n->next = m->next;
    }
}

#if 0

void dmem(struct malloc_head *mh)
{
    register struct malloc_hole *m;
    char *status;
        if (mh)m = mh->holes;else m = memmap.holes;
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

static struct malloc_hole *best_fit_hole(register struct malloc_hole *m, segext_t size)
{
    register struct malloc_hole *best = NULL;

    do {
	if ((m->flags == HOLE_FREE) && (m->extent >= size) &&
		    (!best || best->extent > m->extent))
	    best = m;
    } while ((m = m->next));
    return best;
}

/*
 *	Find the hole starting at a given location
 */

static struct malloc_hole *find_hole(register struct malloc_hole *m, seg_t base)
{
    while (m && (m->page_base != base)) {
	m = m->next;
    }
    return m;
}

#endif /* CONFIG_MEM_TABLE */


#ifdef CONFIG_MEM_LIST

/* Split a segment if too large */

static seg_t seg_split (seg_t node_seg, seg_t node_size, seg_t req_size)
{
	seg_t split_size;
	seg_t next_seg = 0;

	split_size = req_size + 1;
	if (node_size > split_size) {
		pokew (SEG_HEAD_SIZE, node_seg, req_size);

		next_seg = node_seg + split_size;
		seglist_insert_after (SEG_HEAD_NODE, node_seg, next_seg);

		pokew (SEG_HEAD_SIZE,  next_seg, node_size - split_size);
		pokew (SEG_HEAD_FLAGS, next_seg, 0);  /* free */
		pokew (SEG_HEAD_COUNT, next_seg, 0);
	}

	return next_seg;
}

/* Get a free segment */

static seg_t seg_free_get (seg_t req_size)
{
	word_t node_size;
	word_t node_flags;

	seg_t  best_seg  = 0;
	seg_t  best_size = 0xFFFF;

	seg_t  node_seg = _seg_first;

	/* First get the smallest suitable free segment */

	while (1) {
		node_size  = peekw (SEG_HEAD_SIZE, node_seg);
		node_flags = peekw (SEG_HEAD_FLAGS, node_seg);

		if (!(node_flags & SEG_FLAG_USED) && (node_size >= req_size) && (node_size < best_size)) {
			best_seg  = node_seg;
			best_size = node_size;
		}

		seglist_next (SEG_HEAD_NODE, node_seg, &node_seg);
		if (node_seg == _seg_first) break;
	}

	/* Then allocate that free segment */

	if (best_seg) {
		seg_split (best_seg, best_size, req_size);
		pokew (SEG_HEAD_FLAGS, best_seg, SEG_FLAG_USED);
		pokew (SEG_HEAD_COUNT, best_seg, 1);
	}

	return best_seg;
}

/* Merge two contiguous segments */

static void seg_merge (seg_t first_seg, seg_t next_seg)
{
	seg_t first_size;
	seg_t next_size;

	next_size = peekw (SEG_HEAD_SIZE, next_seg);
	seglist_remove (SEG_HEAD_NODE, next_seg);

	first_size = peekw (SEG_HEAD_SIZE, first_seg) + 1 + next_size;
	pokew (SEG_HEAD_SIZE, first_seg, first_size);
}

/* Merge to left */

static seg_t seg_merge_left (seg_t node_seg)
{
	seg_t left_seg = 0;

	while (1) {
		if (node_seg == _seg_first) break;
		seglist_prev (SEG_HEAD_NODE, node_seg, &left_seg);
		if (peekw (SEG_HEAD_FLAGS, left_seg) & SEG_FLAG_USED) {
			left_seg = 0;
			break;
		}

		seg_merge (left_seg, node_seg);
		break;
	}

	return left_seg;
}

/* Merge to right */

static void seg_merge_right (seg_t node_seg)
{
	seg_t right_seg = 0;

	seglist_next (SEG_HEAD_NODE, node_seg, &right_seg);
	if ((right_seg != _seg_first) && !(peekw (SEG_HEAD_FLAGS, right_seg) & SEG_FLAG_USED))
		seg_merge (node_seg, right_seg);
}

#endif /* CONFIG_MEM_LIST */

/*
 *     Allocate a segment
 */

seg_t mm_alloc(segext_t pages)
{
#ifdef CONFIG_MEM_TABLE
    /*
     *      Which hole fits best ?
     */
    register struct malloc_hole *m;

    m = best_fit_hole(holes, pages);
    if (m == NULL)
	return 0;

    /*
     *      The hole is (probably) too big
     */

    split_hole(&memmap, m, pages);
    m->refcount = 1;

    return m->page_base;

#endif /* CONFIG_MEM_TABLE */

#ifdef CONFIG_MEM_LIST

	seg_t res = seg_free_get (pages);
	if (res) res++;  /* skip header */
	return res;

#endif /* CONFIG_MEM_LIST */
}

#ifdef CONFIG_MEM_LIST

void mm_free (seg_t base)
{
	seg_t left_seg;

	seg_t node_seg = base - 1;  /* back to header */
	left_seg = seg_merge_left (node_seg);
	if (left_seg) {
		node_seg = left_seg;
	} else {
		pokew (SEG_HEAD_FLAGS, node_seg, 0);  /* free */
		pokew (SEG_HEAD_COUNT, node_seg, 0);
	}

	seg_merge_right (node_seg);
}

#endif /* CONFIG_MEM_LIST */

/* 	Increase segment reference count */

void mm_get(seg_t base)
{
#ifdef CONFIG_MEM_TABLE

    register struct malloc_hole *m;

    m = find_hole(holes, base);
    m->refcount++;

    return m->page_base;

#endif /* CONFIG_MEM_TABLE */

#ifdef CONFIG_MEM_LIST

    seg_t node_seg = base - 1;  /* back to header */
    word_t count = peekw (SEG_HEAD_COUNT, node_seg);
    pokew (SEG_HEAD_COUNT, node_seg, count + 1);

#endif /* CONFIG_MEM_LIST */
}

/* Decrease segment reference count */
/* Free the segment on no more reference */

/*
 *	Caution: A process can be killed dead by another.
 *	Be sure to catch that in exit...
 */

void mm_put(seg_t base)
{
#ifdef CONFIG_MEM_TABLE
    register struct malloc_hole *mh = holes;
    register struct malloc_hole *m;

    m = find_hole(mh, base);
    if (!m) {

	panic("mm corruption");

    }

    if (m->flags != HOLE_USED)
	panic("double free");
    if (!(--(m->refcount)))
	free_hole(mh, m);

#endif /* CONFIG_MEM_TABLE */

#ifdef CONFIG_MEM_LIST

	seg_t node_seg = base - 1;  /* back to header */
	word_t count = peekw (SEG_HEAD_COUNT, node_seg);
	if (count > 1)
		pokew (SEG_HEAD_COUNT, node_seg, count - 1);
	else
		mm_free (base);

#endif /* CONFIG_MEM_LIST */
}

/*
 *	Allocate a segment and copy it from another
 */

seg_t mm_dup(seg_t base)
{
#ifdef CONFIG_MEM_TABLE

    register struct malloc_hole *o;
    register char *mbase;

    debug("MALLOC: mm_dup()\n");
    o = find_hole(holes, base);
    if (o->flags != HOLE_USED)
	panic("bad hole");

    if ((mbase = (char *)mm_alloc(o->extent)) != NULL)
	fmemcpyb(NULL, (seg_t) mbase, NULL, o->page_base, (word_t) (o->extent << 4));
    return (seg_t)mbase;

#endif /* CONFIG_MEM_TABLE */

#ifdef CONFIG_MEM_LIST

	seg_t new_seg;

	seg_t old_seg = base - 1;  /* back to header */
	seg_t old_size = peekw (SEG_HEAD_SIZE, old_seg);
	new_seg = seg_free_get (old_size);
	if (new_seg)
		fmemcpyb (NULL, ++new_seg, 0, base, old_size << 4);

    return new_seg;

#endif /* CONFIG_MEM_TABLE */
}

/*
 * Returns memory usage information in KB's.
 * "type" is either MM_MEM and "used"
 * selects if we request the used or free memory.
 */
unsigned int mm_get_usage(int type, int used)
{
#ifdef CONFIG_MEM_TABLE

    register struct malloc_hole *m;
    unsigned int ret = 0;

    m = holes;

    used = used ? HOLE_USED : HOLE_FREE;

    do {
	if ((int)m->flags == used)
	    ret += m->extent;
    } while ((m = m->next));

    return ret >> 6;

#endif /* CONFIG_MEM_TABLE */

#ifdef CONFIG_MEM_LIST

	seg_t  node_size;
	word_t node_flags;

	word_t res = 0;
	seg_t  node_seg = _seg_first;

	if (type == MM_MEM) {
		while (1) {
			node_size  = peekw (SEG_HEAD_SIZE, node_seg);
			node_flags = peekw (SEG_HEAD_FLAGS, node_seg);
			/*if (used) printk ("seg %X: size %X used %d count %d\n",
				node_seg, node_size, node_flags, peekw (SEG_HEAD_COUNT, node_seg));*/
			if ((node_flags & SEG_FLAG_USED) == used)
				res += (node_size >> 6);
			seglist_next (SEG_HEAD_NODE, node_seg, &node_seg);
			if (node_seg == _seg_first) break;
		}
	}

	return res;

#endif /* CONFIG_MEM_LIST */
}

#ifdef CONFIG_MEM_TABLE

/*
 * Resize a hole
 */
static struct malloc_hole *mm_resize(register struct malloc_hole *m, segext_t pages)
{
    register struct malloc_hole *next;
    segext_t ext;

    if (m->extent >= pages) {
        /* for now don't reduce holes */
        return m;
    }
    ext = pages - m->extent;

    next = m->next;		/* First try extending to the next hole */
    if (next && (next->flags == HOLE_FREE) && (next->extent >= ext)) {
        m->extent += ext;
        next->page_base += ext;
        if ((next->extent -= ext) == 0) {
            next->flags = HOLE_SPARE;
            m->next = next->next;
        }
        return m;
    }

#ifdef CONFIG_ADVANCED_MM
    /* Next, try relocating to a larger hole */
    if ((m->refcount == 1) && ((next = (struct malloc_hole *)mm_alloc(pages)) != NULL)) {
	fmemcpyb(NULL, (seg_t) next, 0, m->page_base, (word_t) (m->extent << 4));
	next = find_hole(holes, (seg_t)next);
	free_hole(holes, m);
	return next;
    }

#endif

    return NULL;
}

#endif /* CONFIG_MEM_TABLE */

#ifdef CONFIG_MEM_LIST

/* Try to change the size of an allocated segment */

seg_t mm_realloc (seg_t base, seg_t req_size)
{
	seg_t node_size;

	seg_t next_seg;
	seg_t next_size;

	seg_t node_seg = base - 1;  /* back to header */

	while (1) {
		node_size = peekw (SEG_HEAD_SIZE, node_seg);
		if (node_size == req_size) break;

		if (node_size > req_size) {
			next_seg = seg_split (node_seg, node_size, req_size);
			if (next_seg) {
				pokew (SEG_HEAD_FLAGS, next_seg, 0);  /* free */
				pokew (SEG_HEAD_COUNT, next_seg, 0);
				seg_merge_right (next_seg);
			}

			break;
		}

		seglist_next (SEG_HEAD_NODE, node_seg, &next_seg);
		if ((next_seg == _seg_first) || (peekw (SEG_HEAD_FLAGS, next_seg) & SEG_FLAG_USED)) {
			node_seg = 0;
			break;
		}

		req_size = req_size - node_size - 1;
		next_size = peekw (SEG_HEAD_SIZE, next_seg);
		if (next_size < req_size) {
			node_seg = 0;
			break;
		}

		seg_split (next_seg, next_size, req_size);
		seg_merge (node_seg, next_seg);
		break;
	}

	if (node_seg) node_seg++;  /* skip header */
	return node_seg;
}

#endif /* CONFIG_MEM_LIST */


int sys_brk(__pptr newbrk)
{
    register __ptask currentp = current;

	/*printk("brk(%d): new %x, edat %x, ebrk %x, free %x sp %x, eseg %x, %d/%dK\n",
		current->pid, newbrk, currentp->t_enddata, currentp->t_endbrk,
		currentp->t_regs.sp - currentp->t_endbrk,
		currentp->t_regs.sp, currentp->t_endseg,
		mm_get_usage(MM_MEM, 1), mm_get_usage(MM_MEM, 0));*/

    if (newbrk < currentp->t_enddata)
        return -ENOMEM;

    if (currentp->t_begstack > currentp->t_endbrk) {	/* Old format? */
        if (newbrk > currentp->t_endseg - MIN_STACK_SIZE) {	/* Yes */
			printk("sys_brk failed: brk %x > endseg %x\n",
				newbrk, currentp->t_endseg - MIN_STACK_SIZE);
            return -ENOMEM;
		}
    }
#ifdef CONFIG_EXEC_ELKS
    if (newbrk > currentp->t_endseg) {
#ifdef CONFIG_MEM_LIST
        return -ENOMEM;
#else /* CONFIG_MEM_TABLE */
        /* Resize time */
		struct malloc_hole *h = find_hole(holes, currentp->mm.dseg);

        h = mm_resize(h, (newbrk + 15) >> 4);
        if (!h)
            return -ENOMEM;

		currentp->t_regs.ds = currentp->t_regs.es = currentp->t_regs.ss
			= currentp->mm.dseg = h->page_base;
		currentp->t_endseg = newbrk;
#endif /* CONFIG_MEM_TABLE */

    }
#endif /* CONFIG_EXEC_ELKS */
    currentp->t_endbrk = newbrk;

    return 0;
}


int sys_sbrk (int increment, u16_t * pbrk)
{
	__pptr brk = current->t_endbrk;		/* always return start of old break*/
	if (increment) {
		int err = sys_brk(brk + increment);
		if (err) return err;
	}

	put_user (brk, pbrk);
	return 0;
}


/*
 *	Initialise the memory manager.
 */

void mm_init(seg_t start, seg_t end)
{
#ifdef CONFIG_MEM_TABLE
    register struct malloc_hole *holep = &holes[MAX_SEGMENTS - 1];

    /*
     *      Mark pages free.
     */
    do {
	holep->flags = HOLE_SPARE;
    } while (--holep > holes);

    /*
     *      Single hole containing all user memory.
     */
    holep->flags = HOLE_FREE;
    holep->page_base = start;
    holep->extent = end - start;
    holep->refcount = 0;
    holep->next = NULL;

#endif /* CONFIG_MEM_TABLE */

#ifdef CONFIG_MEM_LIST

    _seg_first = start;
    seglist_init (SEG_HEAD_NODE, start);            /* seg_head.node */
    pokew (SEG_HEAD_SIZE,  start, end - start -1);  /* seg_head.size */
    pokew (SEG_HEAD_FLAGS, start, 0);               /* seg_head.flags = free */
    pokew (SEG_HEAD_COUNT, start, 0);               /* seg_head.count = 0 */

#endif /* CONFIG_MEM_LIST */
}

