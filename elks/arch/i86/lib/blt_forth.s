!
! blt_forth( soff, sseg, doff, dseg, bytes )
! for to > from
!
	.globl _blt_forth

_blt_forth:
	mov	ax,si
	mov	dx,di
	mov	si,sp
	mov	bx,es
	mov	cx, 10[si]
	les	di, 6[si]
	lds	si, 2[si]
	std
	rep
	movsb
	mov	di,dx
	mov	si,ax
	mov	ax,ss
	mov	ds,ax
	mov	es,bx
	ret
