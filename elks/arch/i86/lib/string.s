! string.s
! Contributed by Christoph Niemann <niemann@swt.ruhr-uni-bochum.de>

	.globl _strlen
	.globl _strcpy
	.globl _strcmp
	.globl _memset
	.text
	.even

!
! int strlen(char *str)
!
! returns the length of a string in ax
!

_strlen:			! needs more testing!
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
	ret

!
! char *strcpy(char *dest, char *source)
!
! copies the zero terminated string source to dest.
!

_strcpy:
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
	ret

!
! int strcmp(char *str1, char *str2)
!
! compares to zero terminated strings
!

_strcmp:
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
	ret

!
! char *memset(char *p, int value, size_t count)
!

_memset:
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
	ret
