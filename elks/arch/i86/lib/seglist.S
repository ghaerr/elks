/* Double-linked list with segments */

/* void seglist_init (word_t root_off, seg_t root_seg); */
	.global _seglist_init

_seglist_init:
	push  bp
	mov   bp,sp
	push  ds
	lds   bx,[bp+4]     /* node = root */
	mov   [bx],ds       /* node->prev = node */
	mov   [bx+2],ds     /* node->next = node */
	pop   ds
	pop   bp
	ret

seglist_link:
	mov   ds,cx         /* prev->next = node */
	mov   [bx+2],ax
	mov   ds,dx         /* next->prev = node */
	mov   [bx],ax
	mov   ds,ax
	mov   [bx],cx       /* node->prev = prev */
	mov   [bx+2],dx     /* node->next = next */
	ret

/* void seglist_insert_before (word_t node_off, seg_t next_seg, seg_t node_seg); */
	.global _seglist_insert_before

_seglist_insert_before:
	push  bp
	mov   bp,sp
	push  ds
	mov   bx,[bp+4]     /* ax:bx = node */
	mov   ax,[bp+8]
	mov   dx,[bp+6]     /* dx:bx = next */
	mov   ds,dx         /* prev = next->prev */
	mov   cx,[bx]       /* cx:bx = prev */
	call  seglist_link
	pop   ds
	pop   bp
	ret

/* void seglist_insert_after (word_t node_off, seg_t prev_seg, seg_t node_seg); */
	.global _seglist_insert_after

_seglist_insert_after:
	push  bp
	mov   bp,sp
	push  ds
	mov   bx,[bp+4]     /* ax:bx = node */
	mov   ax,[bp+8]
	mov   cx,[bp+6]     /* cx:bx = prev */
	mov   ds,cx         /* next = prev->next */
	mov   dx,[bx+2]     /* dx:bx = next */
	call  seglist_link
	pop   ds
	pop   bp
	ret

/* void seglist_remove (word_t node_off, seg_t node_seg); */
	.global _seglist_remove

_seglist_remove:
	push  bp
	mov   bp,sp
	push  ds
	mov   bx,[bp+4]     /* ax:bx = node */
	mov   ax,[bp+6]
	mov   ds,ax
	mov   cx,[bx]       /* prev = cx:bx = node->prev */
	mov   dx,[bx+2]     /* next = dx:bx = node->next */
	mov   ds,cx         /* prev->next = next */
	mov   [bx+2],dx
	mov   ds,dx         /* next->prev = prev */
	mov   [bx],cx
	pop   ds
	pop   bp
	ret

/* void seglist_prev (word_t node_off, seg_t node_seg, seg_t * prev_seg); */
	.global _seglist_prev

_seglist_prev:
	push  bp
	mov   bp,sp
	push  ds
	mov   bx,[bp+4]     /* ax:bx = node */
	mov   ax,[bp+6]
	mov   ds,ax
	mov   cx,[bx]       /* prev = cx:bx = node->prev */
	pop   ds
	mov   bx,[bp+8]
	mov   [bx],cx
	pop   bp
	ret

/* void seglist_next (word_t node_off, seg_t node_seg, seg_t * next_seg); */
	.global _seglist_next

_seglist_next:
	push  bp
	mov   bp,sp
	push  ds
	mov   bx,[bp+4]     /* ax:bx = node */
	mov   ax,[bp+6]
	mov   ds,ax
	mov   dx,[bx+2]     /* next = dx:bx = node->next */
	pop   ds
	mov   bx,[bp+8]
	mov   [bx],dx
	pop   bp
	ret
