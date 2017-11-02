; word_t peekw (word_t off, seg_t seg)
; segment after offset to allow LDS from the stack

	.text

	.globl _peekw

_peekw:
	push   bp
	mov    bp,sp
	push   ds
	push   bx
	lds    bx,[bp+4]  ; arg0+1: far pointer
	mov    ax,[bx]    ; DS by default
	pop    bx
	pop    ds
	pop    bp
	ret
