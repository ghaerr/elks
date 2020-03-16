// Kernel library
// Local heap management

#pragma once

#include <linuxmt/types.h>
#include <linuxmt/list.h>

#define HEAP_DEBUG

// Heap block header

#define HEAP_TAG_USED 0x80

struct heap {
	list_s node;
	word_t size;
	byte_t tag;
};

typedef struct heap heap_s;

// Heap functions

void * heap_alloc (word_t size);
void heap_free (void * data);

void heap_add (void * data, word_t size);

#ifdef HEAP_DEBUG
void heap_iterate (int (* cb) (heap_s * h));
#endif /* HEAP_DEBUG */
