; void pokew (word_t off, seg_t seg, word_t val)
; segment after offset to allow LDS from the stack
; writes the word at the far pointer segment:offset

	.text

	.define _pokew

_pokew:
	mov    bx,sp
	mov    cx,ds
	mov    ax,[bx+6]  ; arg2: value
	lds    bx,[bx+2]  ; arg0+1: far pointer
	mov    [bx],ax    ; DS by default
	mov    ds,cx
	ret
