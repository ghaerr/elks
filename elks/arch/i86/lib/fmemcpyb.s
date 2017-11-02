; void fmemcpyb (word_t dst_off, seg_t dst_seg, word_t src_off, seg_t src_seg, word_t count)
; segment after offset to allow LDS & LES from the stack

	.text

	.globl _fmemcpyb

_fmemcpyb:
	push   bp
	mov    bp,sp
	push   es
	push   ds
	push   di
	push   si
	push   cx
	les    di,[bp+4]   ; arg0+1: far destination pointer
	lds    si,[bp+8]   ; arg2+3: far source pointer
	mov    cx,[bp+12]  ; arg4:   word count
	cld
	rep
	movsb
	pop    cx
	pop    si
	pop    di
	pop    ds
	pop    es
	pop    bp
	ret

