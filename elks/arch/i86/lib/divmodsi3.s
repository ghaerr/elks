
	.text
	.extern	ldivmod
	.extern	ludivmod

	.globl	___divsi3
	.even

___divsi3:
	push	bp
	mov	bp,sp
	push	bx
	push	cx
	push	di
	mov	ax,4[bp]
	mov	bx,6[bp]
	mov	cx,8[bp]
	mov	di,10[bp]
	call	ldivmod
	mov	dx,di
	mov	ax,cx
	pop	di
	pop	cx
	pop	bx
	pop	bp
	ret

	.globl	___modsi3
	.even

___modsi3:
	push	bp
	mov	bp,sp
	push	bx
	push	cx
	push	di
	mov	ax,4[bp]
	mov	bx,6[bp]
	mov	cx,8[bp]
	mov	di,10[bp]
	call	ldivmod
	mov	dx,bx
	pop	di
	pop	cx
	pop	bx
	pop	bp
	ret

	.globl	___udivsi3
	.even

___udivsi3:
	push	bp
	mov	bp,sp
	push	bx
	push	cx
	push	di
	mov	ax,4[bp]
	mov	bx,6[bp]
	mov	cx,8[bp]
	mov	di,10[bp]
	call	ludivmod
	mov	dx,di
	mov	ax,cx
	pop	di
	pop	cx
	pop	bx
	pop	bp
	ret

	.globl	___umodsi3
	.even

___umodsi3:
	push	bp
	mov	bp,sp
	push	bx
	push	cx
	push	di
	mov	ax,4[bp]
	mov	bx,6[bp]
	mov	cx,8[bp]
	mov	di,10[bp]
	call	ludivmod
	mov	dx,bx
	pop	di
	pop	cx
	pop	bx
	pop	bp
	ret
