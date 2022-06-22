//------------------------------------------------------------------------------
// #include <string.h>
// void * memcmp (const void * s1, const void * s2, size_t n);
//------------------------------------------------------------------------------

#include <libc-private/call-cvt.h>

	.code16

	.text

	.global memcmp

memcmp:
	push %bp
	mov %sp,%bp

	// Save SI DI ES

	mov %es,%dx

	mov %ds,%ax
	mov %ax,%es

	mov %si,%cx
	mov %di,%bx

	// Do the copy

	mov 4+FAR_ADJ_(%bp),%si  // s1
	mov 6+FAR_ADJ_(%bp),%di  // s2
	mov 8+FAR_ADJ_(%bp),%cx  // n

	xor %ax,%ax
	cld
	rep
	cmpsb
	je 1f		// all the same
	sbb %ax,%ax
	sbb $-1,%ax	// choose +/-1
1:

	// Restore SI DI ES

	mov %cx,%si
	mov %bx,%di
	mov %dx,%es

	pop %bp
	RET_(6)

//------------------------------------------------------------------------------
