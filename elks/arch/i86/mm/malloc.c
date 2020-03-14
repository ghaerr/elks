/*
 *	Memory management support.
 */

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
//#include <linuxmt/mem.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>
#include <asm/seglist.h>

#define MIN_STACK_SIZE 0x1000	/* 4k min stack above heap*/

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

/*
 *     Allocate a segment
 */

seg_t mm_alloc(segext_t pages)
{
	seg_t res = seg_free_get (pages);
	if (res) res++;  /* skip header */
	return res;
}

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

/* 	Increase segment reference count */

void mm_get(seg_t base)
{
    seg_t node_seg = base - 1;  /* back to header */
    word_t count = peekw (SEG_HEAD_COUNT, node_seg);
    pokew (SEG_HEAD_COUNT, node_seg, count + 1);
}

/* Decrease segment reference count */
/* Free the segment on no more reference */

/*
 *	Caution: A process can be killed dead by another.
 *	Be sure to catch that in exit...
 */

void mm_put(seg_t base)
{
	seg_t node_seg = base - 1;  /* back to header */
	word_t count = peekw (SEG_HEAD_COUNT, node_seg);
	if (count > 1)
		pokew (SEG_HEAD_COUNT, node_seg, count - 1);
	else
		mm_free (base);
}

/*
 *	Allocate a segment and copy it from another
 */

seg_t mm_dup(seg_t base)
{
	seg_t new_seg;

	seg_t old_seg = base - 1;  /* back to header */
	seg_t old_size = peekw (SEG_HEAD_SIZE, old_seg);
	new_seg = seg_free_get (old_size);
	if (new_seg)
		fmemcpyb (NULL, ++new_seg, 0, base, old_size << 4);

    return new_seg;
}

/*
 * Returns memory usage information in KB's.
 * "type" is either MM_MEM and "used"
 * selects if we request the used or free memory.
 */
unsigned int mm_get_usage(int type, int used)
{
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
}

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
        return -ENOMEM;
    }
#endif /* CONFIG_EXEC_ELKS */
    currentp->t_endbrk = newbrk;

    return 0;
}


int sys_sbrk (int increment, __u16 * pbrk)
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
    _seg_first = start;
    seglist_init (SEG_HEAD_NODE, start);            /* seg_head.node */
    pokew (SEG_HEAD_SIZE,  start, end - start -1);  /* seg_head.size */
    pokew (SEG_HEAD_FLAGS, start, 0);               /* seg_head.flags = free */
    pokew (SEG_HEAD_COUNT, start, 0);               /* seg_head.count = 0 */
}

