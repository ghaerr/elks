// Kernel library
// Local heap management

#pragma once

#include <linuxmt/types.h>
#include <linuxmt/list.h>

/*#define HEAP_DEBUG*/

// Heap block header

// Tag bits 0-6 help for statistics

#define HEAP_TAG_FREE    0x00
#define HEAP_TAG_USED    0x80
#define HEAP_TAG_TYPE    0x0F
#define HEAP_TAG_SEG     0x01
#define HEAP_TAG_STRING  0x02
#define HEAP_TAG_TTY     0x03

// TODO: move free list node from header to body
// to reduce overhead for allocated block

struct heap {
	list_s all;
	list_s free;
	word_t size;
	byte_t tag;
	byte_t unused;
};

typedef struct heap heap_s;

// Heap data

extern list_s _heap_all;

// Heap functions

void * heap_alloc (word_t size, byte_t tag);
void heap_free (void * data);

void heap_add (void * data, word_t size);
void heap_init ();

#ifdef HEAP_DEBUG
void heap_iterate (void (* cb) (heap_s * h));
#endif /* HEAP_DEBUG */
