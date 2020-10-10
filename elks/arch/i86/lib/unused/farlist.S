/* Double-linked list with far pointers */

/* void farlist_init (word_t root_off, seg_t root_seg); */
	.global _farlist_init

_farlist_init:
	push  bp
	mov   bp,sp
	push  ds
	lds   bx,[bp+4]     /* node = root */
	mov   [bx],bx       /* node->prev = node */
	mov   [bx+2],ds
	mov   [bx+4],bx     /* node->next = node */
	mov   [bx+6],ds
	pop   ds
	pop   bp
	ret

farlist_link:
	mov   ds,cx         /* prev->next = node */
	mov   [si+4],bx
	mov   [si+6],ax
	mov   ds,dx         /* next->prev = node */
	mov   [di],bx
	mov   [di+2],ax
	mov   ds,ax
	mov   [bx],si       /* node->prev = prev */
	mov   [bx+2],cx
	mov   [bx+4],di     /* node->next = next */
	mov   [bx+6],dx
	ret

/* void insert_before (word_t next_off, seg_t next_seg, word_t node_off, seg_t node_seg); */
	.global _farlist_insert_before

_farlist_insert_before:
	push  bp
	mov   bp,sp
	push  si
	push  di
	push  ds
	mov   bx,[bp+8]     /* ax:bx = node */
	mov   ax,[bp+10]
	mov   di,[bp+4]     /* dx:di = next */
	mov   dx,[bp+6]
	mov   ds,dx         /* prev = next->prev */
	mov   si,[di]       /* cx:si = prev */
	mov   cx,[di+2]
	call  farlist_link
	pop   ds
	pop   di
	pop   si
	pop   bp
	ret

/* void insert_after (word_t prev_off, seg_t prev_seg, word_t node_off, seg_t node_seg); */
	.global _farlist_insert_after

_farlist_insert_after:
	push  bp
	mov   bp,sp
	push  si
	push  di
	push  ds
	mov   bx,[bp+8]     /* ax:bx = node */
	mov   ax,[bp+10]
	mov   si,[bp+4]     /* cx:si = prev */
	mov   cx,[bp+6]
	mov   ds,cx         /* next = dx:di = prev->next */
	mov   di,[si+4]
	mov   dx,[si+6]
	call  farlist_link
	pop   ds
	pop   di
	pop   si
	pop   bp
	ret

/* void farlist_remove (word_t node_off, seg_t node_seg); */
	.global _farlist_remove

_farlist_remove:
	push  bp
	mov   bp,sp
	push  si
	push  di
	push  ds
	mov   bx,[bp+4]     /* ax:bx = node */
	mov   ax,[bp+6]
	mov   ds,ax
	mov   si,[bx]       /* prev = cx:si = node->prev */
	mov   cx,[bx+2]
	mov   di,[bx+4]     /* next = dx:di = node->next */
	mov   dx,[bx+6]
	mov   ds,cx         /* prev->next = next */
	mov   [si+4],di
	mov   [si+6],dx
	mov   ds,dx         /* next->prev = prev */
	mov   [di],si
	mov   [di+2],cx
	pop   ds
	pop   di
	pop   si
	pop   bp
	ret

/* void farlist_prev (word_t node_off, seg_t node_seg, word_t * prev_off, seg_t * prev_seg); */
	.global _farlist_prev

_farlist_prev:
	push  bp
	mov   bp,sp
	push  si
	push  ds
	mov   bx,[bp+4]     /* ax:bx = node */
	mov   ax,[bp+6]
	mov   ds,ax
	mov   si,[bx]       /* prev = cx:si = node->prev */
	mov   cx,[bx+2]
	pop   ds
	mov   bx,[bp+8]
	mov   [bx],si
	mov   bx,[bp+10]
	mov   [bx],cx
	pop   si
	pop   bp
	ret

/* void farlist_next (word_t node_off, seg_t node_seg, word_t * next_off, seg_t * next_seg); */
	.global _farlist_next

_farlist_next:
	push  bp
	mov   bp,sp
	push  di
	push  ds
	mov   bx,[bp+4]     /* ax:bx = node */
	mov   ax,[bp+6]
	mov   ds,ax
	mov   di,[bx+4]     /* next = dx:di = node->next */
	mov   dx,[bx+6]
	pop   ds
	mov   bx,[bp+8]
	mov   [bx],di
	mov   bx,[bp+10]
	mov   [bx],dx
	pop   di
	pop   bp
	ret
