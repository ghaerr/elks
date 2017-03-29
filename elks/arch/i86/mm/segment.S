/*
 *	Assembly user access routines for the kernel.
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
	cld
	rep
	movsb
	mov	di,dx
	mov	si,ax
	mov	ax,ss
	mov	ds,ax
	ret

/* void memcpy_tofs(void *daddr, void *saddr, size_t len);*/
	.globl	_memcpy_tofs

_memcpy_tofs:
	mov	ax,si
	mov	dx,di
	mov	bx,sp
	mov	di,2[bx]
	mov	si,4[bx]
	mov	cx,6[bx]
	mov	bx,_current
	mov	es,TASK_USER_DS[bx]
	cld
	rep
	movsb
	mov	di,dx
	mov	si,ax
	mov	ax,ss
	mov	es,ax
	ret

/* void fmemcpy(dseg, dest, sseg, src, size); */
	.globl	_fmemcpy

_fmemcpy:
	mov	ax,si
	mov	dx,di
	mov	bx,sp
	mov	es,2[bx]
	mov	si,8[bx]
	mov	cx,10[bx]
	lds	di,4[bx]
	cld
	rep
	movsb
	mov	di,dx
	mov	si,ax
	mov	ax,ss
	mov	ds,ax
	mov	es,ax
	ret

/* int strlen_fromfs(void *saddr); */

    /*  scasb uses es:di, not ds:si, so it is not necessary
     *  to save and restore ds
     */
	.globl	_strlen_fromfs

_strlen_fromfs:
	mov	bx,_current
	mov	es,TASK_USER_DS[bx]
	mov	dx,di
	mov	bx,sp
	mov	di,2[bx]
	xor	al,al		! search for NULL byte
	cld
	mov	cx,#-1
	repne
	scasb
	mov	ax,ss
	mov	es,ax
	mov	ax,di		! calc len +1
	mov	di,dx
	sub	ax,2[bx]
	dec	ax
	ret

	.data
	.extern	_current

