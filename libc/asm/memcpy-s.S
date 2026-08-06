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

	// Copy words, then the odd trailing byte if there is one.
	//
	// REP MOVSB moves one byte per 17 clocks on an 8088; REP MOVSW moves
	// two per 25, so words are ~26% cheaper and there is no misalignment
	// penalty on an 8088 (the bus is 8 bits wide either way). On a real
	// 8086 an aligned word move is cheaper still and a misaligned one
	// costs the same as two bytes, so this never loses.
	//
	// SHR puts the odd-length bit in CF; REP MOVSW leaves flags alone and
	// ends with CX = 0; RCL then reconstitutes CX = 0 or 1. This is the
	// same sequence the kernel has used in memcpy_fromfs/memcpy_tofs
	// (elks/arch/i86/mm/memcpyfs.S) for years. n = 0 falls through both
	// REPs without copying.

	cld
	shr	$1,%cx			// copy words
	rep
	movsw
	rcl	$1,%cx			// then possibly final byte
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
