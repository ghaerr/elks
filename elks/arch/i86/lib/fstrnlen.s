; word_t fstrnlen (word_t off, seg_t seg, word_t max)
; segment after offset to enable LES from the stack

	.text

	.globl fstrnlen

_fstrnlen:
	push   bp
	mov    bp,sp
	push   es
	push   di
	push   cx
	les    di,[bp+4]  ; arg0+1: far pointer
	mov    cx,[bp+8]  ; arg2:   max bytes
	xor    al,al      ; scan for zero byte
	cld
	repne
	scasb
	jnz    fstrnlen_max
	mov    ax,di
	sub    ax,[bp+4]
	dec    ax
	jmp    fstrnlen_exit

fstrnlen_max:
	mov    ax,#0xFFFF  ; maximum reach

fstrnlen_exit:
	pop    cx
	pop    di
	pop    es
	pop    bp
	ret
