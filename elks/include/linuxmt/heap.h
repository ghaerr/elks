#ifndef __LINUXMT_HEAP_H
#define __LINUXMT_HEAP_H

// Kernel library
// Local heap management

#include <linuxmt/types.h>
#include <linuxmt/list.h>

/*#define HEAP_DEBUG*/

// Heap block header

// Tag bits 0-6 help for statistics

#define HEAP_TAG_FREE    0x00
#define HEAP_TAG_USED    0x80
#define HEAP_TAG_CLEAR   0x40	/* return cleared memory*/
#define HEAP_TAG_TYPE    0x0F
#define HEAP_TAG_SEG     0x01
#define HEAP_TAG_STRING  0x02	/* unused*/
#define HEAP_TAG_TTY     0x03
#define HEAP_TAG_INTHAND 0x04
#define HEAP_TAG_BUFHEAD 0x05
#define HEAP_TAG_PIPE    0x06


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

#endif
