/*
 *	Memory management support.
 */

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
//#include <linuxmt/mem.h>
#include <linuxmt/debug.h>
#include <linuxmt/heap.h>

#include <arch/segment.h>

#define MIN_STACK_SIZE 0x1000	/* 4k min stack above heap*/

// TODO: reduce size

// Segment descriptor

// Allocated in local memory for speed
// and to ease the 286 protected mode
// whenever that mode comes back one day


// TODO: locking
static segment_s * _seg_first;


// Split segment if enough large

static int seg_split (segment_s * s1, segext_t size0)
{
	segext_t size2 = s1->size - size0;

	if (size2 > 0 /* SEG_MIN_SIZE */) {

		// TODO: use pool_alloc
		segment_s * s2 = (segment_s *) heap_alloc (sizeof (segment_s), HEAP_TAG_SEG);
		if (!s2) return -ENOMEM;

		s2->base = s1->base + size0;
		s2->size = size2;
		s2->flags = 0;  // free
		s2->ref_count = 0;

		list_insert_after (&s1->node, &s2->node);

		s1->size = size0;
	}

	return 0;  // success
}


// Get free segment

static segment_s * seg_free_get (segext_t size0)
{
	segment_s * best_seg  = 0;
	segext_t best_size = 0xFFFF;

	if (!_seg_first) return 0;
	segment_s * seg = _seg_first;

	// First get the smallest suitable free segment
	// TODO: improve speed with free list

	while (1) {
		segext_t size1 = seg->size;
		if (!(seg->flags & SEG_FLAG_USED) && (size1 >= size0) && (size1 < best_size)) {
			best_seg  = seg;
			best_size = size1;
			if (size1 == size0) break;
		}

		seg = structof (seg->node.next, segment_s, node);
		if (seg == _seg_first) break;
	}

	// Then allocate that free segment

	if (best_seg) {
		int err = seg_split (best_seg, size0);
		if (err)
			printk ("seg:cannot split:heap full");

		best_seg->flags = SEG_FLAG_USED;
		best_seg->ref_count = 1;
	}

	return best_seg;
}


// Merge two contiguous segments

static void seg_merge (segment_s * s1, segment_s * s2)
{
	list_remove (&s2->node);
	s1->size += s2->size;
	heap_free (s2);
}


// Try to merge with previous segment

static segment_s * seg_merge_prev (segment_s * seg)
{
	if (seg == _seg_first) return seg;
	segment_s * prev = structof (seg->node.prev, segment_s, node);
	if (prev->flags & SEG_FLAG_USED) return seg;
	seg_merge (prev, seg);
	return prev;
		}


// Try to merge with next segment

static void seg_merge_right (segment_s * seg)
{
	segment_s * next = structof (seg->node.next, segment_s, node);
	if (next != _seg_first && !(next->flags & SEG_FLAG_USED))
		seg_merge (seg, next);
}


// Allocate segment

segment_s * seg_alloc (segext_t size)
{
	segment_s * seg = 0;
	//lock_wait (&_seg_lock);
	seg = seg_free_get (size);
	//unlock_event (&_seg_lock);
	return seg;
}


// Free segment

void seg_free (segment_s * seg1)
{
	//lock_wait (&_seg_lock);
	segment_s * seg2 = seg_merge_prev (seg1);
	if (seg1 == seg2)  // no segment merge
		seg1->flags = 0;  // free

	seg_merge_right (seg2);
	//unlock_event (&_seg_lock);
	}


// Increase segment reference count

segment_s * seg_get (segment_s * seg)
{
	// TODO: atomic increment
	seg->ref_count++;
	return seg;
}


// Decrease segment reference count */
// Free segment on no more reference

void seg_put (segment_s * seg)
{
	// TODO: atomic decrement
	if (!--seg->ref_count)
		seg_free (seg);
}


// Allocate segment and copy from another
// Typically used by process fork

segment_s * seg_dup (segment_s * src)
{
	segment_s * dst = seg_free_get (src->size);
	if (dst)
		fmemcpyb (NULL, dst->base, 0, src->base, src->size << 4);

	return dst;
}


// Get memory information (free or used) in KB

unsigned int mm_get_usage(int type, int used)
{
	segment_s * seg = _seg_first;
	if (!_seg_first) return 0;

	word_t res = 0;

	if (type == MM_MEM) {
		while (1) {
			/*if (used) printk ("seg %X: size %u used %u count %u\n",
				seg->base, seg->size, seg->flags, seg->ref_count);*/

			if ((seg->flags & SEG_FLAG_USED) == used)
				res += seg->size >> 6;

			seg = structof (seg->node.next, segment_s, node);
			if (seg == _seg_first) break;
		}
	}

	return res;
}


// User data segment functions

int sys_brk(__pptr newbrk)
{
    register __ptask currentp = current;

	/*printk("brk(%d): new %x, edat %x, ebrk %x, free %x sp %x, eseg %x, %d/%dK\n",
		current->pid, newbrk, currentp->t_enddata, currentp->t_endbrk,
		currentp->t_regs.sp - currentp->t_endbrk,
		currentp->t_regs.sp, currentp->t_endseg,
		mm_get_usage(MM_MEM, SEG_FLAG_USED), mm_get_usage(MM_MEM, 0));*/

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


// Initialize the memory manager.

void mm_init(seg_t start, seg_t end)
{
	segment_s * seg = (segment_s *) heap_alloc (sizeof (segment_s), HEAP_TAG_SEG);
	if (seg) {
		seg->base = start;
		seg->size = end - start;
		seg->flags = 0;  // free
		seg->ref_count = 0;

		list_init (&seg->node);
		_seg_first = seg;
	}
}

