; word_t peekw (word_t off, seg_t seg)
; segment after offset to allow LDS from the stack
; returns the word at the far pointer segment:offset

	.text

	.define _peekw

_peekw:
	mov    dx,ds
	mov    bx,sp
	lds    bx,[bx+2]  ; arg0+1: far pointer
	mov    ax,[bx]  ; DS by default
	mov    ds,dx
	ret
