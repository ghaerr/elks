! lmodul.s
! unsigned bx:ax / 2(di):(di), remainder bx:ax,quotient di:cx, dx not preserved

	.globl	lmodul
	.extern	ludivmod
	.text
	.even

lmodul:
	mov	cx,[di]
	mov	di,2[di]
	call	ludivmod	! unsigned bx:ax / di:cx, quot di:cx, rem bx:ax
	ret
