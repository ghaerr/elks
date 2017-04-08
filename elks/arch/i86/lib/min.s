! __u16 min(__u16 x, __u16 y);

	.globl	_min
	.text
	.even

_min:
	pop	bx
	pop	ax	! x
	pop	dx	! y
	cmp	ax,dx
	jbe	.0
	mov	ax,dx
.0:
	sub	sp,*4
	jmp	bx
