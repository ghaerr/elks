! setjmp.s

	.define	_setjmp
	.define	_longjmp

BP_OFF		=	0
SP_OFF		=	2
IP_OFF		=	4

	.text
	.even

_setjmp:
	pop	cx		! ip
	pop	bx		! pointer to jmp_buf
	dec	sp
	dec	sp
	mov	BP_OFF(bx),bp
	mov	SP_OFF(bx),sp
	mov	IP_OFF(bx),cx
	xor	ax,ax		! non-jump return
	jmp	cx

	.even
_longjmp:
	pop	bx		! junk ip
	pop	bx		! pointer to jmp_buf
	pop	ax		! value (sp now junk, don't fake args)
	cmp	ax,#1		! fixup 0 value to 1 (avoiding branches)
	adc	al,#0		! change ax to 1 iff ax was 0

! the  setjmp.s  in  MINIX 1.2  backtraces the frame pointers here
! this can't be done with more efficient nonstandard frames

	mov	bp,BP_OFF(bx)
	mov	sp,SP_OFF(bx)
	jmp	IP_OFF(bx)
