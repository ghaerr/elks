! void pokew( unsigned segment, unsigned *offset, unsigned long value );
! writes the word value  at the far pointer  segment:offset

	.define _poked
	.text
	.even

_poked:
	mov	cx,ds		! save current data segment
	pop	dx		! get return address
	pop	ds		! get poke segment
	pop	bx		! get poke offset
	pop	ax		! get high word of long
	mov	[bx+2],ax	! and store it. 
	pop	ax		! get low word of long
	mov	[bx],ax		! and store it. 
	sub	sp,*8		! adjust stack pointer for return
	mov	ds,cx		! restore current data segment
	jmp	dx		! and return
