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

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov %es,%dx

	mov %ds,%ax
	mov %ax,%es

	mov %di,%bx
#else
	push %es
	push %di

	mov %ds,%bx
	mov %bx,%es
#endif

	// Do the setup

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov 4+FAR_ADJ_(%bp),%di  // s
	mov 6+FAR_ADJ_(%bp),%ax  // c
	mov 8+FAR_ADJ_(%bp),%cx  // n
#else
	mov %ax,%di  // s
	mov %ax,%bx
	mov %dx,%ax  // c
		     // n = CX already
#endif

	cld
	rep
	stosb

	// Restore DI ES

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov %bx,%di
	mov %dx,%es
#else
	pop %di
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
