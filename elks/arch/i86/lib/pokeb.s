; void pokeb (word_t off, seg_t seg, byte__t val)
; segment after offset to allow LDS from the stack
; BCC pushes byte_t as word_t

	.text

	.globl _pokeb

_pokeb:
	push   bp
	mov    bp,sp
	push   ds
	push   bx
	lds    bx,[bp+4]  ; arg0+1: far pointer
	mov    ax,[bp+8]  ; arg2: value
	mov    [bx],al    ; DS by default
	pop    bx
	pop    ds
	pop    bp
	ret
