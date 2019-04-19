//------------------------------------------------------------------------------
// #include <string.h>
// void * memset (void * s, int c, size_t n);
//------------------------------------------------------------------------------

	.code16

	.text

	.global memset

memset:
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %bp
	mov %sp,%bp

	// Save DI ES

	mov %es,%dx

	mov %ds,%ax
	mov %ax,%es

	mov %di,%bx

	// Do the setup

	mov 4(%bp),%di  // s
	mov 6(%bp),%ax  // c
	mov 8(%bp),%cx  // n

	cld
	rep
	stosb

	// Restore DI ES

	mov %bx,%di
	mov %dx,%es

	// Return value is destination

	mov 4(%bp),%ax

# ifdef __IA16_CALLCVT_STDCALL
	ret $6
# else
	ret
# endif
#else
	push %ax
	mov %es, %bx

	push %ds
	pop %es
	xchg %ax, %di
	xchg %ax, %dx

	cld
	rep
	stosb

	mov %dx, %di
	mov %bx, %es
	pop %ax
	ret
#endif

//------------------------------------------------------------------------------
