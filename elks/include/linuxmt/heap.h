#ifndef __LINUXMT_HEAP_H
#define __LINUXMT_HEAP_H

// Kernel library
// Local heap management

#include <linuxmt/types.h>
#include <linuxmt/list.h>

// Heap block header

// Tag bits 0-6 help for statistics

#define HEAP_TAG_FREE    0x00
#define HEAP_TAG_USED    0x80
#define HEAP_TAG_CLEAR   0x40	/* return cleared memory*/
#define HEAP_TAG_TYPE    0x0F
#define HEAP_TAG_SEG     0x01   /* main memory segment */
#define HEAP_TAG_DRVR    0x02   /* DF floppy or driver buffers */
#define HEAP_TAG_TTY     0x03   /* open tty in/out queues */
#define HEAP_TAG_TASK    0x04   /* task array */
#define HEAP_TAG_BUFHEAD 0x05   /* buffer heads */
#define HEAP_TAG_PIPE    0x06   /* open pipe buffers */
#define HEAP_TAG_INODE   0x07   /* system inodes */
#define HEAP_TAG_FILE    0x08   /* system open files */


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
void heap_iterate (void (* cb) (heap_s * h));

#endif
