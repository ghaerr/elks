; void fmemsetw (word_t off, seg_t seg, word_t val, word_t count)
; segment after offset to allow LES from the stack

	.text

	.globl _fmemsetw

_fmemsetw:
	push   bp
	mov    bp,sp
	push   es
	push   di
	push   cx
	les    di,[bp+4]   ; arg0+1: far pointer
	mov    ax,[bp+8]   ; arg2:   value
	mov    cx,[bx+10]  ; arg3:   word count
	cld
	rep
	stosw
	pop    cx
	pop    di
	pop    es
	pop    bp
	ret
