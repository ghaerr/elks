/* Double-linked list with segments */

	.code16

	.text

/* void seglist_init (word_t root_off, seg_t root_seg); */
	.global seglist_init

seglist_init:
	push  %bp
	mov   %sp,%bp
	push  %ds
	lds   4(%bp),%bx     /* node = root */
	mov   %ds,(%bx)       /* node->prev = node */
	mov   %ds,2(%bx)     /* node->next = node */
	pop   %ds
	pop   %bp
	ret

seglist_link:
	mov   %cx,%ds         /* prev->next = node */
	mov   %ax,2(%bx)
	mov   %dx,%ds         /* next->prev = node */
	mov   %ax,(%bx)
	mov   %ax,%ds
	mov   %cx,(%bx)       /* node->prev = prev */
	mov   %dx,2(%bx)     /* node->next = next */
	ret

/* void seglist_insert_before (word_t node_off, seg_t next_seg, seg_t node_seg); */
	.global seglist_insert_before

seglist_insert_before:
	push  %bp
	mov   %sp,%bp
	push  %ds
	mov   4(%bp),%bx     /* ax:bx = node */
	mov   8(%bp),%ax
	mov   6(%bp),%dx     /* dx:bx = next */
	mov   %dx,%ds         /* prev = next->prev */
	mov   (%bx),%cx       /* cx:bx = prev */
	call  seglist_link
	pop   %ds
	pop   %bp
	ret

/* void seglist_insert_after (word_t node_off, seg_t prev_seg, seg_t node_seg); */
	.global seglist_insert_after

seglist_insert_after:
	push  %bp
	mov   %sp,%bp
	push  %ds
	mov   4(%bp),%bx     /* ax:bx = node */
	mov   8(%bp),%ax
	mov   6(%bp),%cx     /* cx:bx = prev */
	mov   %cx,%ds         /* next = prev->next */
	mov   2(%bx),%dx     /* dx:bx = next */
	call  seglist_link
	pop   %ds
	pop   %bp
	ret

/* void seglist_remove (word_t node_off, seg_t node_seg); */
	.global seglist_remove

seglist_remove:
	push  %bp
	mov   %sp,%bp
	push  %ds
	mov   4(%bp),%bx     /* ax:bx = node */
	mov   6(%bp),%ax
	mov   %ax,%ds
	mov   (%bx),%cx       /* prev = cx:bx = node->prev */
	mov   2(%bx),%dx     /* next = dx:bx = node->next */
	mov   %cx,%ds         /* prev->next = next */
	mov   %dx,2(%bx)
	mov   %dx,%ds         /* next->prev = prev */
	mov   %cx,(%bx)
	pop   %ds
	pop   %bp
	ret

/* void seglist_prev (word_t node_off, seg_t node_seg, seg_t * prev_seg); */
	.global seglist_prev

seglist_prev:
	push  %bp
	mov   %sp,%bp
	push  %ds
	mov   4(%bp),%bx     /* ax:bx = node */
	mov   6(%bp),%ax
	mov   %ax,%ds
	mov   (%bx),%cx       /* prev = cx:bx = node->prev */
	pop   %ds
	mov   8(%bp),%bx
	mov   %cx,(%bx)
	pop   %bp
	ret

/* void seglist_next (word_t node_off, seg_t node_seg, seg_t * next_seg); */
	.global seglist_next

seglist_next:
	push  %bp
	mov   %sp,%bp
	push  %ds
	mov   4(%bp),%bx     /* ax:bx = node */
	mov   6(%bp),%ax
	mov   %ax,%ds
	mov   2(%bx),%dx     /* next = dx:bx = node->next */
	pop   %ds
	mov   8(%bp),%bx
	mov   %dx,(%bx)
	pop   %bp
	ret
