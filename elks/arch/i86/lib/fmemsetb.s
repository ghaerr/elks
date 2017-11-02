; void fmemsetb (word_t off, seg_t seg, byte_t val, word_t count)
; segment after offset to allow LES from the stack
; BCC pushes byte_t as word_t

	.text

	.globl _fmemsetb

_fmemsetb:
	push   bp
	mov    bp,sp
	push   es
	push   di
	push   cx
	les    di,[bp+4]   ; arg0+1: far pointer
	mov    ax,[bp+8]   ; arg2:   value
	mov    cx,[bx+10]  ; arg3:   byte count
	cld
	rep
	stosb
	pop    cx
	pop    di
	pop    es
	pop    bp
	ret
