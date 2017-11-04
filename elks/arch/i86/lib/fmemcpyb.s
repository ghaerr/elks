; void fmemcpyb (word_t dst_off, seg_t dst_seg, word_t src_off, seg_t src_seg, word_t count)
; segment after offset to allow LDS & LES from the stack
; assume DS=ES=SS (not ES for GCC-IA16)

	.text

	.define _fmemcpyb

_fmemcpyb:
#ifdef USE_IA16
	mov    bx,es
#endif
	mov    ax,si
	mov    dx,di
	mov    si,sp
	mov    cx,[si+10]  ; arg4:   word count
	les    di,[si+2]   ; arg0+1: far destination pointer
	lds    si,[si+6]   ; arg2+3: far source pointer
	cld
	rep
	movsb
	mov    si,ax
	mov    di,dx
	mov    ax,ss
	mov    ds,ax
#ifdef USE_IA16
	mov    es,bx
#else
	mov    es,ax
#endif
	ret
