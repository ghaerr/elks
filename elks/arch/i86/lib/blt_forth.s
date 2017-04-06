!
! blt_forth( soff, sseg, doff, dseg, bytes )
! for to > from
!
	.globl _blt_forth

_blt_forth:
	mov	bx,sp
	mov	ax,si
	mov	dx,di
	lds	si, 2[bx]
	les	di, 6[bx]
	mov	cx, 10[bx]
	std
	rep
	movsb
	mov	di,dx
	mov	si,ax
	mov	ax,ss
	mov	ds,ax
	mov	es,ax
	ret
