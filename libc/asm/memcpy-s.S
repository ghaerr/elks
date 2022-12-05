//------------------------------------------------------------------------------
// #include <string.h>
// void * memcpy (void * dest, const void * src, size_t n);
//------------------------------------------------------------------------------

#include <libc-private/call-cvt.h>

	.arch	i8086, nojumps
	.code16

	.text

	.global memcpy

memcpy:
	push %bp
	mov %sp,%bp

	// Save SI DI ES

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov %es,%dx

	mov %ds,%ax
	mov %ax,%es

	mov %si,%ax
	mov %di,%bx
#else
	push %es

	mov %ds,%bx
	mov %bx,%es

	push %si
	push %di
#endif

	// Do the copy

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov 4+FAR_ADJ_(%bp),%di  // dest
	mov 6+FAR_ADJ_(%bp),%si  // src
	mov 8+FAR_ADJ_(%bp),%cx  // n
#else
	mov %ax,%di  // dest
	mov %ax,%bx
	mov %dx,%si  // src
		     // n = CX already
#endif

	cld
	rep
	movsb

	// Restore SI DI ES

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov %ax,%si
	mov %bx,%di

	mov %dx,%es
#else
	pop %di
	pop %si

	pop %es
#endif

	// Return value is destination

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov 4+FAR_ADJ_(%bp),%ax
#else
	mov %bx,%ax
#endif

	pop %bp
	RET_(6)

//------------------------------------------------------------------------------
