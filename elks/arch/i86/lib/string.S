// string.s
// Contributed by Christoph Niemann <niemann@swt.ruhr-uni-bochum.de>

    .code16

	.text

	.global strlen
	.global strcpy
	.global strcmp
	.global memset

/*
! int strlen(char *str)
!
! returns the length of a string in ax
!
*/

strlen:			// needs more testing!
#ifdef USE_IA16
	mov	%es,%dx
	mov	kernel_ds,%es
#endif
	mov	%sp,%bx
	push	%di
	mov	2(%bx),%di
	xor	%al,%al
	cld			// so di increments not decrements
	mov	$-1,%cx		// maximum loop count
	repne			// while (cx) {
	scasb			//     cx--; if (al - [es:di++] == 0) break;}
	mov	%di,%ax
	sub	2(%bx),%ax
	dec	%ax
	pop	%di
#ifdef USE_IA16
	mov	%dx,%es
#endif
	ret

/*
!
! char *strcpy(char *dest, char *source)
!
! copies the zero terminated string source to dest.
!
*/

strcpy:
#ifdef USE_IA16
	mov	%es,%dx
	mov	kernel_ds,%es
#endif
	mov	%sp,%bx
	push	%di
	push	%si
	mov	2(%bx),%di	// address of the destination string
	mov	4(%bx),%si	// address of the source string
	cld

copyon:	lodsb			// al = [ds:si++]
	stosb			// [es:di++] = al
	test	%al,%al
	jnz	copyon
	mov	2(%bx),%ax	// _strcpy returns a pointer to the destination string
	pop	%si
	pop	%di
#ifdef USE_IA16
	mov	%dx,%es
#endif
	ret

/*
!
! int strcmp(char *str1, char *str2)
!
! compares to zero terminated strings
!
*/

strcmp:
#ifdef USE_IA16
	mov	%es,%dx
	mov	kernel_ds,%es
#endif
	mov	%sp,%bx
	push	%di
	push	%si
	mov	2(%bx),%si	// address of the string 1
	mov	4(%bx),%di	// address of the string 2
	cld

cmpon:	lodsb			// al = [ds:si++]
	scasb			// al - [es:di++]
	jne	cmpend
	test	%al,%al
	jnz	cmpon
	xor	%ax,%ax		// both strings are the same
	jmp	cmpret

cmpend:	sbb	%ax,%ax		// strings differ
	or	$1,%ax

cmpret:	pop	%si
	pop	%di
#ifdef USE_IA16
	mov	%dx,%es
#endif
	ret

/*
!
! char *memset(char *p, int value, size_t count)
!
*/

memset:
#ifdef USE_IA16
	mov	%es,%dx
	mov	kernel_ds,%es
#endif
	mov	%sp,%bx
	push	%di
	mov	2(%bx),%di	// address of the memory block
	mov	4(%bx),%ax	// byte to write
	mov	6(%bx),%cx	// loop count
	cld
	rep			// while (cx)
	stosb			// 	cx--, [es:di++] = al
	mov	2(%bx),%ax	// return value = start addr of block
	pop	%di
#ifdef USE_IA16
	mov	%dx,%es
#endif
	ret

#ifdef USE_IA16
	.data
	.extern kernel_ds
#endif
