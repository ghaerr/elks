; int fmemcmpw (word_t dst_off, seg_t dst_seg, word_t src_off, seg_t src_seg, word_t count)
; segment after offset to allow LDS & LES from the stack
; assume DS=ES=SS (not ES for GCC-IA16)

	.text

	.define _fmemcmpw

_fmemcmpw:
#ifdef USE_IA16
	mov    bx,es
#endif
	mov    ax,si
	mov    dx,di
	mov    si,sp
	mov    cx,[si+10]  ; arg4:   byte count
	les    di,[si+2]   ; arg0+1: far destination pointer
	lds    si,[si+6]   ; arg2+3: far source pointer
	cld
	repz
	cmpsw
	mov    si,ax
	mov    di,dx
	jz     fmemcmpw_same
	mov    ax,#1
	jmp    fmemcmpw_exit

fmemcmpw_same:
	xor    ax,ax

fmemcmpw_exit:
	mov    dx,ss
	mov    ds,dx
#ifdef USE_IA16
	mov    es,bx
#else
	mov    es,dx
#endif
	ret
