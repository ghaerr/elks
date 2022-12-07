//------------------------------------------------------------------------------
// #include <string.h>
// char * strcpy (char * dest, const char * src);
//------------------------------------------------------------------------------

#include <libc-private/call-cvt.h>

	.code16

	.text

	.global strcpy

strcpy:
	push %bp
	mov %sp,%bp

	// Save SI DI ES

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov %es,%dx

	mov %ds,%ax
	mov %ax,%es

	mov %si,%bx
	mov %di,%cx
#else
	push %es

	mov %ds,%bx
	mov %bx,%es

	push %si
	mov %di,%cx
#endif

	// Do the copy

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov 4+FAR_ADJ_(%bp),%di  // dest
	mov 6+FAR_ADJ_(%bp),%si  // src
#else
	mov %ax,%di  // dest
	mov %ax,%bx
	mov %dx,%si  // src
#endif
	cld

_loop:
	lodsb
	stosb
	test %al,%al
	jnz _loop

	// Restore SI DI ES

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov %bx,%si
	mov %cx,%di

	mov %dx,%es
#else
	pop %si
	mov %cx,%di

	pop %es
#endif

	// Return value is destination

#ifndef __IA16_CALLCVT_REGPARMCALL
	mov 4+FAR_ADJ_(%bp),%ax
#else
	mov %bx,%ax
#endif

	pop %bp
	RET_(4)

//------------------------------------------------------------------------------

