; int memcmp (void *dst, void *src, size_t len)
; TODO: return +1 / -1 cases for standard behavior

	.text

	.define _memcmp

_memcmp:
#ifdef USE_IA16
	mov    bx,es
	mov	   ax,ds
	mov    es,ax
#endif
	mov    ax,si
	mov    dx,di
	mov    si,sp
	mov    cx,[si+6]  ; arg2: byte count
	mov    di,[si+2]  ; arg0: destination pointer
	mov    si,[si+4]  ; arg1: source pointer
	cld
	repz
	cmpsb
	mov    si,ax
	mov    di,dx
	jz     memcmp_same
	mov    ax,#1
	jmp    memcmp_exit

memcmp_same:
	xor    ax,ax

memcmp_exit:
#ifdef USE_IA16
	mov    es,bx
#endif
	ret
