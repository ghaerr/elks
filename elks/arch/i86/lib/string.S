! string.s
! Contributed by Christoph Niemann <niemann@swt.ruhr-uni-bochum.de>

	.globl _strlen
	.globl _strcpy
	.globl _strcmp
	.globl _memset
	.globl _memcmp
	.text
	.even

!
! int strlen(char *str)
!
! returns the length of a string in ax
!

_strlen:			! needs more testing!
#ifdef USE_IA16
	mov	dx,es
	mov	es,_kernel_ds
#endif
	mov	bx,sp
	push	di
	mov	di,2[bx]
	xor	al,al
	cld			! so di increments not decrements
	mov	cx,#-1		! maximum loop count
	repne			! while (cx) {
	scasb			!     cx--; if (al - [es:di++] == 0) break;}
	mov	ax,di
	sub	ax,2[bx]
	dec	ax
	pop	di
#ifdef USE_IA16
	mov	es,dx
#endif
	ret

!
! char *strcpy(char *dest, char *source)
!
! copies the zero terminated string source to dest.
!

_strcpy:
#ifdef USE_IA16
	mov	dx,es
	mov	es,_kernel_ds
#endif
	mov	bx,sp
	push	di
	push	si
	mov	di,2[bx]	! address of the destination string
	mov	si,4[bx]	! address of the source string
	cld

copyon:	lodsb			! al = [ds:si++]
	stosb			! [es:di++] = al
	test	al,al
	jnz	copyon
	mov	ax,2[bx]	! _strcpy returns a pointer to the destination string
	pop	si
	pop	di
#ifdef USE_IA16
	mov	es,dx
#endif
	ret

!
! int strcmp(char *str1, char *str2)
!
! compares to zero terminated strings
!

_strcmp:
#ifdef USE_IA16
	mov	dx,es
	mov	es,_kernel_ds
#endif
	mov	bx,sp
	push	di
	push	si
	mov	si,2[bx]	! address of the string 1
	mov	di,4[bx]	! address of the string 2
	cld

cmpon:	lodsb			! al = [ds:si++]
	scasb			! al - [es:di++]
	jne	cmpend
	testb	al,al
	jnz	cmpon
	xor	ax,ax		! both strings are the same
	jmp	cmpret

cmpend:	sbb	ax,ax		! strings differ
	or	ax,#1

cmpret:	pop	si
	pop	di
#ifdef USE_IA16
	mov	es,dx
#endif
	ret

!
! char *memset(char *p, int value, size_t count)
!

_memset:
#ifdef USE_IA16
	mov	dx,es
	mov	es,_kernel_ds
#endif
	mov	bx,sp
	push	di
	mov	di,2[bx]	! address of the memory block
	mov	ax,4[bx]	! byte to write
	mov	cx,6[bx]	! loop count
	cld
	rep			! while (cx)
	stosb			! 	cx--, [es:di++] = al
	mov	ax,2[bx]	! return value = start addr of block
	pop	di
#ifdef USE_IA16
	mov	es,dx
#endif
	ret


/*
 *  int memcmp (void *dst, void *src, size_t len);
 */

_memcmp:
	push   bp
	mov    bp,sp
	push   di
	push   si
	push   cx
	mov    di,[bp+4]           /* arg0: destination pointer */
	mov    si,[bp+6]           /* arg1: source pointer */
	mov    cx,[bp+8]           /* arg2: byte count */
	cld
	repz
	cmpsb
	jz     memcmp_same
	mov    ax,#1
	jmp    memcmp_exit

memcmp_same:
	xor    ax,ax

memcmp_exit:
	pop    cx
	pop    si
	pop    di
	pop    bp
	ret


#ifdef USE_IA16
	.data
	.extern _kernel_ds
#endif
