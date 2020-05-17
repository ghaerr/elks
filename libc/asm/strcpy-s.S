//------------------------------------------------------------------------------
// #include <string.h>
// char * strcpy (char * dest, const char * src);
//------------------------------------------------------------------------------

#include <libc-private/call-cvt.h>

	.code16

	.text

	.global strcpy

strcpy:
	push %bp
	mov %sp,%bp

	// Save SI DI ES

	mov %es,%dx

	mov %ds,%ax
	mov %ax,%es

	mov %si,%bx
	mov %di,%cx

	// Do the copy

	mov 4+FAR_ADJ_(%bp),%di  // dest
	mov 6+FAR_ADJ_(%bp),%si  // src
	cld

_loop:
	lodsb
	stosb
	test %al,%al
	jnz _loop

	// Restore SI DI ES

	mov %bx,%si
	mov %cx,%di

	mov %dx,%es

	// Return value is destination

	mov 4+FAR_ADJ_(%bp),%ax

	pop %bp
	RET_(4)

//------------------------------------------------------------------------------

