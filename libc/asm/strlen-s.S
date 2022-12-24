//------------------------------------------------------------------------------
// #include <string.h>
// size_t strlen (const char * s);
//------------------------------------------------------------------------------

#include <libc-private/call-cvt.h>

	.arch	i8086, nojumps
	.code16

	.text

	.global strlen

strlen:
	push %bp
	mov %sp,%bp

	// Save DI ES

	mov %es,%dx

	mov %ds,%bx
	mov %bx,%es

	mov %di,%bx

	// Do the scan

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov 4+FAR_ADJ_(%bp),%di  // s
#else
	mov %ax,%di
#endif
	mov $-1,%cx
	xor %ax,%ax

	cld
	repne
	scasb

	mov %cx,%ax
	not %ax
	dec %ax

	// Restore DI ES

	mov %bx,%di
	mov %dx,%es

	pop %bp
	RET_(2)

//------------------------------------------------------------------------------

