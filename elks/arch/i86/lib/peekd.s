! long peekd( unsigned segment, unsigned int *offset );
! returns the dword at the far pointer  segment:offset

	.define _peekd
	.text
	.even

_peekd:
	mov cx,ds		! save current data segment
	pop dx			! get return address and discard
	pop ds			! get peek segment
	pop bx			! get peek offset
	sub sp,*6		! set stack pointer to return address
	mov ax,[bx]		! get low word of long
	mov dx,[bx+2]		! get high word of long allowing for 
	mov bx,dx		! bcc convention for returning longs
	mov ds,cx		! restore original data segment
	ret			! and return
