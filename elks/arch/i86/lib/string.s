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
	push	bp
	mov	bp,sp
	push	di
	mov	dx,[bp+4]
	mov	di,dx
	xor	al,al
	cld			! so di increments not decrements
	mov	cx,#-1		! maximum loop count
	repne			! while (cx) {
	scasb			!     cx--; if (al - [es:di++] == 0) break;}
	sub	di,dx
	mov	ax,di
	dec	ax
	pop	di
	pop	bp
	ret

!
! char *strcpy(char *dest, char *source)
!
! copies the zero terminated string source to dest.
!

_strcpy:
	push	bp
	mov	bp,sp
	push	di
	push	si
	mov	di,[bp+4]	! address of the destination string
	mov	si,[bp+6]	! address of the source string
	cld

copyon:	lodsb			! al = [ds:si++]
	stosb			! [es:di++] = al
	test	al,al
	jnz	copyon
	mov	ax,[bp+4]	! _strcpy returns a pointer to the destination string
	pop	si
	pop	di
	pop	bp
	ret

!
! int strcmp(char *str1, char *str2)
!
! compares to zero terminated strings
!

_strcmp:
	push	bp
	mov	bp,sp
	push	di
	push	si
	mov	si,[bp+4]	! address of the string 1
	mov	di,[bp+6]	! address of the string 2
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
	pop	bp
	ret

!
! char *memset(char *p, int value, size_t count)
!

_memset:
	push	bp
	mov	bp,sp
	push	di
	mov	di,[bp+4]	! address of the memory block
	mov	ax,[bp+6]	! byte to write
	mov	cx,[bp+8]	! loop count
	cld
	rep			! while (cx)
	stosb			! 	cx--, [es:di++] = al
	mov	ax,[bp+4]	! return value = start addr of block
	pop	di
	pop	bp
	ret
