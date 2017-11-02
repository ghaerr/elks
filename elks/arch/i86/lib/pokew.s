; void pokew (word_t off, seg_t seg, word_t val)
; segment after offset to allow LDS from the stack

	.text

	.globl _pokew

_pokew:
	push   bp
	mov    bp,sp
	push   ds
	push   bx
	lds    bx,[bp+4]  ; arg0+1: far pointer
	mov    ax,[bp+8]  ; arg2: value
	mov    [bx],ax    ; DS by default
	pop    bx
	pop    ds
	pop    bp
	ret
