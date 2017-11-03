; void pokeb (word_t off, seg_t seg, byte__t val)
; segment after offset to allow LDS from the stack
; compiler pushes byte_t as word_t
; writes the byte at the far pointer segment:offset

	.text

	.define _pokeb

_pokeb:
	mov    bx,sp
	mov    cx,ds
	mov    ax,[bx+6]  ; arg2: value
	lds    bx,[bx+2]  ; arg0+1: far pointer
	mov    [bx],al    ; DS by default
	mov    ds,cx
	ret
