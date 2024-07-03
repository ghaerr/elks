/*
 *	Memory management support.
 */

#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
#include <linuxmt/errno.h>
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
		s2->pid = 0;

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
	s1->pid = 0;
	heap_free (s2);
}


// Allocate segment

segment_s * seg_alloc (segext_t size, word_t type)
{
	segment_s * seg = 0;
	seg = seg_free_get (size, type);
	if (seg && (type & SEG_FLAG_ALIGN1K))
		seg->base += ((~seg->base + 1) & ((1024 >> 4) - 1));
	return seg;
}


// Free segment

void seg_free (segment_s * seg)
{
	// Free segment will be inserted to free list:
	//   - tail if merged to previous or next free segment
	//   - head if still alone to increase 'exact hit'
	//     chance on next allocation of same size

	list_s * i = &_seg_free;
	seg->flags = SEG_FLAG_FREE;
	seg->pid = 0;

	// Try to merge with previous segment if free
	list_s * p = seg->all.prev;
	if (&_seg_all != p) {
		segment_s * prev = structof (p, segment_s, all);
		if ((prev->flags == SEG_FLAG_FREE) && (prev->base + prev->size == seg->base)) {
			list_remove (&(prev->free));
			seg_merge (prev, seg);
			i = _seg_free.prev;
			seg = prev;
		}
	}

	// Try to merge with next segment if free

	list_s * n = seg->all.next;
	if (n != &_seg_all) {
		segment_s * next = structof (n, segment_s, all);
		if ((next->flags == SEG_FLAG_FREE) && (seg->base + seg->size == next->base)) {
			list_remove (&(next->free));
			seg_merge (seg, next);
			i = _seg_free.prev;
		}
	}

	// Insert to free list head or tail

	list_insert_after (i, &(seg->free));
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

int sys_brk(segoff_t newbrk)
{
	/***unsigned int memfree, memused;
	mm_get_usage(&memfree, &memused);
	printk("brk(%P): new %x, edat %x, ebrk %x, free %x sp %x, eseg %x, %d/%dK\n",
		newbrk, current->t_enddata, current->t_endbrk,
		current->t_regs.sp - current->t_endbrk,
		current->t_regs.sp, current->t_endseg, memfree, memused);***/

    if (newbrk < current->t_enddata)
        return -ENOMEM;

    if (current->t_begstack > current->t_endbrk) {				/* stack above heap?*/
        if (newbrk > current->t_begstack - current->t_minstack) {
			printk("sys_brk(%d) fail: brk %x over by %u bytes\n",
				current->pid, newbrk, newbrk - (current->t_begstack - current->t_minstack));
            return -ENOMEM;
		}
    }
#ifdef CONFIG_EXEC_LOW_STACK
    if (newbrk > current->t_endseg)
        return -ENOMEM;
#endif
    current->t_endbrk = newbrk;

    return 0;
}


int sys_sbrk (int increment, segoff_t *pbrk)
{
	segoff_t brk = current->t_endbrk;   /* always return start of old break*/
	int err;

	debug("sbrk incr %u pointer %04x curbreak %04x\n", increment, pbrk, brk);
	err = verify_area(VERIFY_WRITE, pbrk, sizeof(*pbrk));
	if (err)
		return err;
	if (increment) {
		err = sys_brk(brk + increment);
		if (err) return err;
	}

	put_user (brk, pbrk);
	return 0;
}

// allocate memory for process, return segment
int sys_fmemalloc(int paras, unsigned short *pseg)
{
	segment_s *seg;
	int err;

	err = verify_area(VERIFY_WRITE, pseg, sizeof(*pseg));
	if (err)
		return err;
	seg = seg_alloc((segext_t)paras, SEG_FLAG_FDAT);
	if (!seg)
		return -ENOMEM;
	seg->pid = current->pid;
	put_user(seg->base, pseg);
	return 0;
}

// free all program allocated segments for PID pid
void seg_free_pid(pid_t pid)
{
	list_s *n;

again:
	for (n = _seg_all.next; n != &_seg_all; ) {
		segment_s * seg = structof (n, segment_s, all);

		if (seg->pid == pid) {
			seg_free(seg);
			goto again;		/* free may have changed linked list */
		}
		n = seg->all.next;
	}
}

void INITPROC seg_add(seg_t start, seg_t end)
{
	segment_s * seg = (segment_s *) heap_alloc (sizeof (segment_s), HEAP_TAG_SEG);
	if(seg) {
		seg->base = start;
		seg->size = end - start;
		seg->flags = SEG_FLAG_FREE;
		seg->ref_count = 0;
		seg->pid = 0;

		list_insert_before (&_seg_all, &(seg->all));  // add tail
		list_insert_before (&_seg_free, &(seg->free));  // add tail
	}
}

// Initialize the memory manager.

void INITPROC mm_init(seg_t start, seg_t end)
{
	list_init (&_seg_all);
	list_init (&_seg_free);

	seg_add(start, end);
}
