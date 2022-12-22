//------------------------------------------------------------------------------
// #include <string.h>
// void * memcmp (const void * s1, const void * s2, size_t n);
//------------------------------------------------------------------------------

#include <libc-private/call-cvt.h>

	.arch	i8086, nojumps
	.code16

	.text

	.global memcmp

memcmp:
	push %bp
	mov %sp,%bp

	// Save SI DI ES

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov %es,%dx

	mov %ds,%ax
	mov %ax,%es

	mov %si,%cx
	mov %di,%bx
#else
	push %es

	mov %ds,%bx
	mov %bx,%es

	push %si
	mov %di,%bx
#endif

	// Do the copy

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov 4+FAR_ADJ_(%bp),%si  // s1
	mov 6+FAR_ADJ_(%bp),%di  // s2
	mov 8+FAR_ADJ_(%bp),%cx  // n
#else
	mov %ax,%si  // s1
	mov %dx,%di  // s2
		     // n = CX already
#endif

	xor %ax,%ax
	cld
	rep
	cmpsb
	je 1f		// all the same
	sbb %ax,%ax
	sbb $-1,%ax	// choose +/-1
1:

	// Restore SI DI ES

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov %cx,%si
	mov %bx,%di
	mov %dx,%es
#else
	pop %si
	mov %bx,%di
	pop %es
#endif

	pop %bp
	RET_(6)

//------------------------------------------------------------------------------
