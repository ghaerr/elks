; byte_t peekb (word_t off, seg_t seg)
; segment after offset to allow LDS from the stack

	.text

	.globl _peekb

_peekb:
	push   bp
	mov    bp,sp
	push   ds
	push   bx
	lds    bx,[bp+4]  ; arg0+1: far pointer
	mov    al,[bx]    ; DS by default */
	xor    ah,ah
	pop    bx
	pop    ds
	pop    bp
	ret
