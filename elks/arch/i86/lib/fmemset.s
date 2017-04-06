!
! void fmemset(char *off, int seg, int value, size_t count)
!

	.globl	_fmemset
	.text

_fmemset:
	mov	bx,sp
	push	es
	push	di
	les	di,2[bx]
	mov	ax,6[bx]
	mov	cx,8[bx]
	cld
	test	cl,1
	jz	___memset_words
	stosb

___memset_words:
	shr	cx,#1
	mov	ah,al
	rep
	stosw
	pop	di
	pop	es
	ret
