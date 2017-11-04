; byte_t peekb (word_t off, seg_t seg)
; segment after offset to allow LDS from the stack
; returns the byte at the far pointer segment:offset

	.text

	.define _peekb

_peekb:
	mov    dx,ds
	mov    bx,sp
	lds    bx,[bx+2]  ; arg0+1: far pointer
	mov    al,[bx]  ; DS by default
	xor    ah,ah
	mov    ds,dx
	ret
