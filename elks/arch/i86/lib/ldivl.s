| ldivl.s
| bx:ax / 2(di):(di), quotient bx:ax, remainder di:cx, dx not preserved

	.globl	ldivl
	.extern	ldivmod
	.text
	.even

ldivl:
	mov	cx,[di]
	mov	di,2[di]
	call	ldivmod		
| bx:ax / di:cx, quot di:cx, rem bx:ax
	xchg	ax,cx
	xchg	bx,di
	ret

