//------------------------------------------------------------------------------
// #include <string.h>
// char * strcpy (char * dest, const char * src);
//------------------------------------------------------------------------------

	.code16

	.text

	.global strcpy

strcpy:
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %bp
	mov %sp,%bp

	// Save SI DI ES

	mov %es,%dx

	mov %ds,%ax
	mov %ax,%es

	mov %si,%bx
	mov %di,%cx

	// Do the copy

	mov 4(%bp),%di  // dest
	mov 6(%bp),%si  // src
	cld

_loop:
	lodsb
	stosb
	test %al,%al
	jnz _loop

	// Restore SI DI ES

	mov %bx,%si
	mov %cx,%di

	mov %dx,%es

	// Return value is destination

	mov 4(%bp),%ax

	pop %bp
# ifdef __IA16_CALLCVT_STDCALL
	ret $4
# else
	ret
# endif
#else
	push %ax
	mov %es, %bx

	mov %ds, %cx
	mov %cx, %es
	mov %di, %cx
	xchg %ax, %di
	xchg %dx, %si
	cld

_loop:
	lodsb
	stosb
	test %al, %al
	jnz _loop

	mov %cx, %di
	mov %dx, %si
	mov %bx, %es
	pop %ax
	ret
#endif

//------------------------------------------------------------------------------

