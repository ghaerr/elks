// Kernel library
// Local heap management

#include <linuxmt/kernel.h>
#include <linuxmt/heap.h>
#include <linuxmt/string.h>

// Minimal block size to hold heap header
// plus enough space in body to be useful
// (= size of the smallest allocation)

#define HEAP_MIN_SIZE (sizeof (heap_s) + 16)

// Heap root

list_s _heap_all;
static list_s _heap_free;


// Split block if enough large

static void heap_split (heap_s * h1, word_t size0)
{
	word_t size2 = h1->size - size0;

	if (size2 >= HEAP_MIN_SIZE) {
		h1->size = size0;

		heap_s * h2 = (heap_s *) ((byte_t *) (h1 + 1) + size0);
		h2->size = size2 - sizeof (heap_s);
		h2->tag = HEAP_TAG_FREE;

		list_insert_after (&(h1->all), &(h2->all));
		list_insert_after (&(h1->free), &(h2->free));
	}
}


// Get free block

static heap_s * free_get (word_t size0, byte_t tag)
{
	// First get the smallest suitable free block

	heap_s * best_h  = 0;
	word_t best_size = 0xFFFF;
	list_s * n = _heap_free.next;

	while (n != &_heap_free) {
		heap_s * h = structof (n, heap_s, free);
		word_t size1  = h->size;

		if ((h->tag == HEAP_TAG_FREE) && (size1 >= size0) && (size1 < best_size)) {
			best_h  = h;
			best_size = size1;
			if (size1 == size0) break;
		}

		n = h->free.next;
	}

	// Then allocate that free block

	if (best_h) {
		heap_split (best_h, size0);
		best_h->tag = HEAP_TAG_USED | tag;
		list_remove (&(best_h->free));
	}

	return best_h;
}


// Merge two contiguous blocks

static void heap_merge (heap_s * h1, heap_s * h2)
{
	h1->size = h1->size + sizeof (heap_s) + h2->size;
	list_remove (&(h2->all));
}


// Allocate block

void * heap_alloc (word_t size, byte_t tag)
{
	heap_s * h = free_get (size, tag);
	if (h) {
		h++;						// skip header
		if (tag & HEAP_TAG_CLEAR)
			memset(h, 0, size);
	}
	if (!h) printk("HEAP: no memory (%u bytes)\n", size);
	return h;
}


// Free block

void heap_free (void * data)
{
	heap_s * h = ((heap_s *) (data)) - 1;  // back to header

	// Free block will be inserted to free list:
	//   - tail if merged to previous or next free block
	//   - head if still alone to increase 'exact hit'
	//     chance on next allocation of same size

	list_s * i = &_heap_free;

	// Try to merge with previous block if free

	list_s * p = h->all.prev;
	if (&_heap_all != p) {
		heap_s * prev = structof (p, heap_s, all);
		if (prev->tag == HEAP_TAG_FREE) {
			list_remove (&(prev->free));
			heap_merge (prev, h);
			i = _heap_free.prev;
			h = prev;
		} else {
			h->tag = HEAP_TAG_FREE;
		}
	}

	// Try to merge with next block if free

	list_s * n = h->all.next;
	if (n != &_heap_all) {
		heap_s * next = structof (n, heap_s, all);
		if (next->tag == HEAP_TAG_FREE) {
			list_remove (&(next->free));
			heap_merge (h, next);
			i = _heap_free.prev;
		}
	}

	// Insert to free list head or tail

	list_insert_after (i, &(h->free));
}


// Add space to heap

void heap_add (void * data, word_t size)
{
	if (size >= HEAP_MIN_SIZE) {
		heap_s * h = (heap_s *) data;
		h->size = size - sizeof (heap_s);
		h->tag = HEAP_TAG_FREE;

		// Add large block to tails of both lists
		// as almost no chance for 'exact hit'

		list_insert_before (&_heap_all, &(h->all));
		list_insert_before (&_heap_free, &(h->free));
	}
}

// Initialize heap

void heap_init ()
{
	list_init (&_heap_all);
	list_init (&_heap_free);
}

#if UNUSED
static void heap_cb (heap_s * h)
{
        printk ("heap:%Xh:%u:%hxh\n",h, h->size, h->tag);
}

void heap_iterate (void (* cb) (heap_s *))
{
	list_s * n = _heap_all.next;

	while (n != &_heap_all) {
		heap_s * h = structof (n, heap_s, all);
		(*cb) (h);
		n = h->all.next;
	}
}
#endif
