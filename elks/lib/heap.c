// Kernel library
// Local heap management

#include <linuxmt/lock.h>
#include <linuxmt/heap.h>
#include <linuxmt/kernel.h>

// Minimal block size to hold header and free list node
#define HEAP_MIN_SIZE (sizeof (heap_s) + sizeof (list_s))

// Heap root
// TODO: regroup in one structure

static lock_t _heap_lock;
heap_s * _heap_first;
// TODO: free block list
//static heap_s * _heap_free;


// Split block if enough large

static void heap_split (heap_s * h1, word_t size0)
{
	word_t size2 = h1->size - size0;

	if (size2 >= HEAP_MIN_SIZE) {
		h1->size = size0;

		heap_s * h2 = (heap_s *) ((byte_t *) (h1 + 1) + size0);
		h2->size = size2 - sizeof (heap_s);
		h2->tag = 0;  // free

		list_insert_after (&(h1->node), &(h2->node));
	}
}


// Get free block

static heap_s * free_get (word_t size0, byte_t tag)
{
	heap_s * best_h  = 0;
	word_t best_size = 0xFFFF;

	if (!_heap_first) return 0;
	heap_s * h = _heap_first;

	// First get the smallest suitable free block
	// TODO: improve speed with free list

	while (1) {
		word_t size1  = h->size;

		if (!(h->tag & HEAP_TAG_USED) && (size1 >= size0) && (size1 < best_size)) {
			best_h  = h;
			best_size = size1;
			if (size1 == size0) break;
		}

		h = structof (h->node.next, heap_s, node);
		if (h == _heap_first) break;
	}

	// Then allocate that free block

	if (best_h) {
		heap_split (best_h, size0);
		best_h->tag = HEAP_TAG_USED | tag;
	}

	return best_h;
}


// Merge two contiguous blocks

static void heap_merge (heap_s * h1, heap_s * h2)
{
	h1->size = h1->size + sizeof (heap_s) + h2->size;
	list_remove (&(h2->node));
}


// Try to heap_merge with previous block

static heap_s * heap_merge_prev (heap_s * h)
{
	if (h == _heap_first) return h;
	heap_s * prev = structof (h->node.prev, heap_s, node);
	if (prev->tag & HEAP_TAG_USED) return h;
	heap_merge (prev, h);
	return prev;
}

// Try to heap_merge with next block

static void heap_merge_next (heap_s * h)
{
	heap_s * next = structof (h->node.next, heap_s, node);
	if ((next != _heap_first) && !((next->tag) & HEAP_TAG_USED))
		heap_merge (h, next);
}


// Allocate block

void * heap_alloc (word_t size, byte_t tag)
{
	wait_lock (&_heap_lock);
	heap_s * h = free_get (size, tag);
	if (h) h++;  // skip header
	event_unlock (&_heap_lock);
	return h;
}

// Free block

void heap_free (void * data)
{
	wait_lock (&_heap_lock);
	heap_s * h1 = ((heap_s *) (data)) - 1;  // back to header
	heap_s * h2 = heap_merge_prev (h1);
	if (h1 == h2)  // no heap_merge
		h1->tag = 0;  // free

	heap_merge_next (h2);
	event_unlock (&_heap_lock);
}

// Add space to heap

void heap_add (void * data, word_t size)
{
	if (size >= HEAP_MIN_SIZE) {
		wait_lock (&_heap_lock);
		heap_s * h = (heap_s *) data;
		h->size = size - sizeof (heap_s);
		h->tag = 0;   // free

		if (_heap_first)
			list_insert_before (&(_heap_first->node), &(h->node));  // add tail
		else {
			list_init (&(h->node));
			_heap_first = h;
		}

	event_unlock (&_heap_lock);
	}
}

// Dump heap

#ifdef HEAP_DEBUG

void heap_iterate (void (* cb) (heap_s *))
{
	if (!_heap_first) return;
	heap_s * h = _heap_first;

	while (1) {
		(*cb) (h);
		h = structof (h->node.next, heap_s, node);
		if (h == _heap_first) break;
	}
}

#endif /* HEAP_DEBUG */

