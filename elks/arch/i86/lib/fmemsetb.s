; void fmemsetb (word_t off, seg_t seg, byte_t val, word_t count)
; segment after offset to allow LES from the stack
; compiler pushes byte_t as word_t

	.text

	.define _fmemsetb

_fmemsetb:
	mov    bx,es
	mov    dx,di
	mov    di,sp
	mov    ax,[di+6]  ; arg2:   value
	mov    cx,[di+8]  ; arg3:   byte count
	les    di,[di+2]  ; arg0+1: far pointer
	cld
	rep
	stosb
	mov    di,dx
	mov    es,bx
	ret
