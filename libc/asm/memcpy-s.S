//------------------------------------------------------------------------------
// #include <string.h>
// void * memcpy (void * dest, const void * src, size_t n);
//------------------------------------------------------------------------------

	.code16

	.text

	.global memcpy

memcpy:
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %bp
	mov %sp,%bp

	// Save SI DI ES

	mov %es,%dx

	mov %ds,%ax
	mov %ax,%es

	mov %si,%ax
	mov %di,%bx

	// Do the copy

	mov 4(%bp),%di  // dest
	mov 6(%bp),%si  // src
	mov 8(%bp),%cx  // n

	cld
	rep
	movsb

	// Restore SI DI ES

	mov %ax,%si
	mov %bx,%di

	mov %dx,%es

	// Return value is destination

	mov 4(%bp),%ax

	pop %bp
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
	xchg %dx, %si

	cld
	rep
	movsb

	xchg %ax, %di
	mov %dx, %si

	mov %bx, %es
	pop %ax
	ret
#endif

//------------------------------------------------------------------------------
