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

// Minimal segment size to be useful
// (= size of the smallest allocation)

#define SEG_MIN_SIZE 1

// Segment descriptor

// Allocated in local memory for speed
// and to ease the 286 protected mode
// whenever that mode comes back one day

static list_s _seg_all;
static list_s _seg_free;


// Split segment if enough large

static segment_s * seg_split (segment_s * s1, segext_t size0)
{
	segext_t size2 = s1->size - size0;

	if (size2 >= SEG_MIN_SIZE) {

		// TODO: use pool_alloc
		segment_s * s2 = (segment_s *) heap_alloc (sizeof (segment_s), HEAP_TAG_SEG);
		if (!s2) {
			printk ("seg:cannot split:heap full\n");
			return 0;
		}

		s2->base = s1->base + size0;
		s2->size = size2;
		s2->flags = SEG_FLAG_FREE;
		s2->ref_count = 0;

		list_insert_after (&s1->all, &s2->all);
		list_insert_after (&s1->free, &s2->free);

		s1->size = size0;

		return s2;	// return upper segment if split
	}

	return s1;		// no split
}


// Get free segment

static segment_s * seg_free_get (segext_t size0, word_t type)
{
	// First get the smallest suitable free segment

	segment_s * best_seg  = 0;
	segext_t best_size = 0xFFFF;
	list_s * n = _seg_free.next;
	segext_t size00 = size0, incr = 0;

	while (n != &_seg_free) {
		segment_s * seg = structof (n, segment_s, free);
		segext_t size1 = seg->size;

		if (type & SEG_FLAG_ALIGN1K)
			size00 = size0 + ((~seg->base + 1) & ((1024 >> 4) - 1));
		if ((seg->flags == SEG_FLAG_FREE) && (size1 >= size00) && (size1 < best_size)) {
			best_seg  = seg;
			best_size = size1;
			incr = size00 - size0;
			if (size1 == size00) break;
		}

		n = seg->free.next;
	}

	// Then allocate that free segment

	if (best_seg) {
		seg_split (best_seg, size00);				// split off upper segment
		if (incr)
			best_seg = seg_split (best_seg, incr);	// split off lower segment

		best_seg->flags = SEG_FLAG_USED | type;
		best_seg->ref_count = 1;
		list_remove (&(best_seg->free));
	}

	return best_seg;
}


// Merge two contiguous segments

static void seg_merge (segment_s * s1, segment_s * s2)
{
	list_remove (&s2->all);
	s1->size += s2->size;
	heap_free (s2);
}


// Allocate segment

segment_s * seg_alloc (segext_t size, word_t type)
{
	segment_s * seg = 0;
	//lock_wait (&_seg_lock);
	seg = seg_free_get (size, type);
	if (seg && (type & SEG_FLAG_ALIGN1K))
		seg->base += ((~seg->base + 1) & ((1024 >> 4) - 1));
	//unlock_event (&_seg_lock);
	return seg;
}


// Free segment

void seg_free (segment_s * seg)
{
	//lock_wait (&_seg_lock);

	// Free segment will be inserted to free list:
	//   - tail if merged to previous or next free segment
	//   - head if still alone to increase 'exact hit'
	//     chance on next allocation of same size

	list_s * i = &_seg_free;

	// Try to merge with previous segment if free

	list_s * p = seg->all.prev;
	if (&_seg_all != p) {
		segment_s * prev = structof (p, segment_s, all);
		if (prev->flags == SEG_FLAG_FREE) {
			list_remove (&(prev->free));
			seg_merge (prev, seg);
			i = _seg_free.prev;
			seg = prev;
		} else {
			seg->flags = SEG_FLAG_FREE;
		}
	}

	// Try to merge with next segment if free

	list_s * n = seg->all.next;
	if (n != &_seg_all) {
		segment_s * next = structof (n, segment_s, all);
		if (next->flags == SEG_FLAG_FREE) {
			list_remove (&(next->free));
			seg_merge (seg, next);
			i = _seg_free.prev;
		}
	}

	// Insert to free list head or tail

	list_insert_after (i, &(seg->free));

	//unlock_event (&_seg_lock);
}


// Increase segment reference count

segment_s * seg_get (segment_s * seg)
{
	seg->ref_count++;
	return seg;
}


// Decrease segment reference count
// Free segment on no more reference

void seg_put (segment_s * seg)
{
	if (!--seg->ref_count)
		seg_free (seg);
}


// Allocate segment and copy from another
// Typically used by process fork

segment_s * seg_dup (segment_s * src)
{
	segment_s * dst = seg_free_get (src->size, src->flags);
	if (dst)
		fmemcpyw(0, dst->base, 0, src->base, src->size << 3);

	return dst;
}


// Get memory information (free and used) in KB

void mm_get_usage (unsigned int * pfree, unsigned int * pused)
{
	unsigned int free = 0;
	unsigned int used = 0;

	list_s * n = _seg_all.next;

	while (n != &_seg_all) {
		segment_s * seg = structof (n, segment_s, all);

		/*if (used) printk ("seg %X: size %u used %u count %u\n",
			seg->base, seg->size, seg->flags, seg->ref_count);*/

		if (seg->flags == SEG_FLAG_FREE)
			free += seg->size;
		else
			used += seg->size;

		n = seg->all.next;
	}

	// Convert paragraphs to kilobytes
	// Floor, not ceiling, so average return

	*pfree = ((free + 31) >> 6);
	*pused = ((used + 31) >> 6);
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

    if (currentp->t_begstack > currentp->t_endbrk) {				/* stack above heap?*/
        if (newbrk > currentp->t_begstack - currentp->t_minstack) {
			printk("sys_brk(%d) fail: brk %x over by %u bytes\n",
				currentp->pid, newbrk, newbrk - (currentp->t_begstack - currentp->t_minstack));
            return -ENOMEM;
		}
    }
#ifdef CONFIG_EXEC_LOW_STACK
    if (newbrk > currentp->t_endseg)
        return -ENOMEM;
#endif
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

void INITPROC mm_init(seg_t start, seg_t end)
{
	list_init (&_seg_all);
	list_init (&_seg_free);

	segment_s * seg = (segment_s *) heap_alloc (sizeof (segment_s), HEAP_TAG_SEG);
	if (seg) {
		seg->base = start;
		seg->size = end - start;
		seg->flags = SEG_FLAG_FREE;
		seg->ref_count = 0;

		list_insert_before (&_seg_all, &(seg->all));  // add tail
		list_insert_before (&_seg_free, &(seg->free));  // add tail
	}
}

