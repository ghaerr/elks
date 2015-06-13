!
! void farmemset(char *off, int seg, int value, size_t count)
!

	.globl _fmemset
	.text

_fmemset:
	push	bp
	mov	bp,sp
	push	es
	push	di
	les	di,[bp+4]
	mov	ax,[bp+8]
	mov	cx,[bp+10]
	cld
	test cl,1
	jz ___memset_words
	stosb

___memset_words:
	shr	cx,#1
	mov	ah,al
	rep
	stosw
	pop	di
	pop	es
	pop	bp
	ret

!
! blt_forth( soff, sseg, doff, dseg, bytes )
! for to > from
!
	.globl _blt_forth

_blt_forth:
	push	bp
	mov	bp, sp
	push	es
	push	ds
	push	si
	push	di
	pushf
	lds	si, [bp+4]
	les	di, [bp+8]
	mov	cx, [bp+12]
	std
	rep
	movsb
	popf
	pop	di
	pop	si
	pop	ds
	pop	es
	pop	bp
	ret

