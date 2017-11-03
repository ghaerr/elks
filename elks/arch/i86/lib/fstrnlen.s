; word_t fstrnlen (word_t off, seg_t seg, word_t max)
; segment after offset to enable LES from the stack

	.text

	.define _fstrnlen

_fstrnlen:
	mov    bx,es
	mov    dx,di
	mov    di,sp
	mov    cx,[di+6]  ; arg2:   max bytes
	les    di,[di+2]  ; arg0+1: far pointer
	xor    al,al      ; scan for zero byte
	cld
	repne
	scasb
	jnz    fstrnlen_max
	mov    ax,di
	mov    di,sp
	sub    ax,[di+2]
	dec    ax
	jmp    fstrnlen_exit

fstrnlen_max:
	mov    ax,#0xFFFF  ; maximum reach

fstrnlen_exit:
	mov    es,bx
	mov    di,dx
	ret
