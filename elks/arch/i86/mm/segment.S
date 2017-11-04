/*
 *  Assembly memory access routines for the kernel.
 */
 
#include <arch/asm-offsets.h>

	.text

/* void memcpy_fromfs(void *daddr, void *saddr, size_t len);*/
	.globl	_memcpy_fromfs

_memcpy_fromfs:
	mov	ax,si
	mov	dx,di
	mov	bx,sp
	mov	di,2[bx]
	mov	si,4[bx]
	mov	cx,6[bx]
	mov	bx,_current
	mov	ds,TASK_USER_DS[bx]
	mov	bx,ss
#ifdef USE_IA16
	push	es
	mov	es,bx
#endif
	cld
	rep
	movsb
#ifdef USE_IA16
	pop	es
#endif
	mov	ds,bx
	mov	di,dx
	mov	si,ax
	ret

/* void memcpy_tofs(void *daddr, void *saddr, size_t len);*/
	.globl	_memcpy_tofs

_memcpy_tofs:
	mov	ax,si
	mov	dx,di
	mov	bx,es
	mov	si,_current
	mov	es,TASK_USER_DS[si]
	mov	si,sp
	mov	di,2[si]
	mov	cx,6[si]
	mov	si,4[si]
	cld
	rep
	movsb
	mov	es,bx
	mov	di,dx
	mov	si,ax
	ret

/* int strnlen_fromfs(void *saddr, size_t maxlen); */

    /*  scasb uses es:di, not ds:si, so it is not necessary
     *  to save and restore ds
     */
	.globl	_strnlen_fromfs

_strnlen_fromfs:
	mov	dx,di
	mov	di,_current
	mov	bx,sp
	push	es
	mov	es,TASK_USER_DS[di]
	mov	di,2[bx]
	mov	cx,4[bx]
	xor	al,al		! search for NULL byte
	cld
	repne
	scasb
	pop	es
	mov	ax,di		! calc len +1
	mov	di,dx
	jnz	strnln1
	dec	ax
strnln1:
	sub	ax,2[bx]
	ret

	.data
	.extern	_current

