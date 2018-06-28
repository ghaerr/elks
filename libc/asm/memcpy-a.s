//------------------------------------------------------------------------------
// #include <string.h>
// void * memcpy (void * dest, const void * src, size_t n);
//------------------------------------------------------------------------------

	.code16

	.text

	.global memcpy

memcpy:
	mov %sp,%bx

	push %si
	push %di

	mov %es,%dx

	mov %ds,%ax
	mov %ax,%es

	mov 2(%bx),%di  // dest
	mov 4(%bx),%si  // src
	mov 6(%bx),%cx  // n

	mov %di,%ax     // retval

	cld
	rep
	movsb

	mov %dx,%es

	pop %di
	pop %si
	ret

//------------------------------------------------------------------------------
