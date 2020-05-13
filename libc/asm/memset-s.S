//------------------------------------------------------------------------------
// #include <string.h>
// void * memset (void * s, int c, size_t n);
//------------------------------------------------------------------------------

#include <libc-private/call-cvt.h>

	.code16

	.text

	.global memset

memset:
	push %bp
	mov %sp,%bp

	// Save DI ES

	mov %es,%dx

	mov %ds,%ax
	mov %ax,%es

	mov %di,%bx

	// Do the setup

	mov 4+FAR_ADJ_(%bp),%di  // s
	mov 6+FAR_ADJ_(%bp),%ax  // c
	mov 8+FAR_ADJ_(%bp),%cx  // n

	cld
	rep
	stosb

	// Restore DI ES

	mov %bx,%di
	mov %dx,%es

	// Return value is destination

	mov 4+FAR_ADJ_(%bp),%ax

	pop %bp
	RET_(6)

//------------------------------------------------------------------------------
