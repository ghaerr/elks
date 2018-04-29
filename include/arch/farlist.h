/* Double-linked list with far pointers */

#ifndef _FARLIST_H
#define _FARLIST_H

#include <arch/types.h>


struct farlist_node_s
	{
	/* far pointers */
	word_t off_prev;
	seg_t  seg_prev;
	word_t off_next;
	seg_t  seg_next;
	};

typedef struct farlist_node_s farlist_node_t;


void farlist_init (word_t node_off, seg_t node_seg);

void farlist_insert_before (word_t next_off, seg_t next_seg, word_t node_off, seg_t node_seg);
void farlist_insert_after  (word_t prev_off, seg_t prev_seg, word_t node_off, seg_t node_seg);

void farlist_remove (word_t node_off, seg_t node_seg);

void farlist_prev (word_t node_off, seg_t node_seg, word_t * prev_off, seg_t * prev_seg);
void farlist_next (word_t node_off, seg_t node_seg, word_t * next_off, seg_t * next_seg);

#endif /* !_FARLIST_H */
