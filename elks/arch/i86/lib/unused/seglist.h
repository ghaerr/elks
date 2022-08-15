/* Double-linked list with segments */

#ifndef _SEGLIST_H
#define _SEGLIST_H

#include <arch/types.h>


struct seglist_node_s
	{
	seg_t  seg_prev;
	seg_t  seg_next;
	};

typedef struct seglist_node_s seglist_node_t;


/* node_off: offset of list node in linked segments */

void seglist_init (word_t root_off, seg_t root_seg);

void seglist_insert_before (word_t node_off, seg_t next_seg, seg_t node_seg);
void seglist_insert_after  (word_t node_off, seg_t prev_seg, seg_t node_seg);

void seglist_remove (word_t node_off, seg_t node_seg);

void seglist_prev (word_t node_off, seg_t node_seg, seg_t * prev_seg);
void seglist_next (word_t node_off, seg_t node_seg, seg_t * next_seg);

#endif /* !_SEGLIST_H */
