//------------------------------------------------------------------------------
// #include <string.h>
// void * memcpy (void * dest, const void * src, size_t n);
//------------------------------------------------------------------------------

#include <libc-private/call-cvt.h>

	.code16

	.text

	.global memcpy

memcpy:
	push %bp
	mov %sp,%bp

	// Save SI DI ES

	mov %es,%dx

	mov %ds,%ax
	mov %ax,%es

	mov %si,%ax
	mov %di,%bx

	// Do the copy

	mov 4+FAR_ADJ_(%bp),%di  // dest
	mov 6+FAR_ADJ_(%bp),%si  // src
	mov 8+FAR_ADJ_(%bp),%cx  // n

	cld
	rep
	movsb

	// Restore SI DI ES

	mov %ax,%si
	mov %bx,%di

	mov %dx,%es

	// Return value is destination

	mov 4+FAR_ADJ_(%bp),%ax

	pop %bp
	RET_(6)

//------------------------------------------------------------------------------
