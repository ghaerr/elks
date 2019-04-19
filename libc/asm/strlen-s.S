//------------------------------------------------------------------------------
// #include <string.h>
// size_t strlen (const char * s);
//------------------------------------------------------------------------------

	.code16

	.text

	.global strlen

strlen:
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %bp
	mov %sp,%bp

	// Save DI ES

	mov %es,%dx

	mov %ds,%ax
	mov %ax,%es

	mov %di,%bx

	// Do the scan

	mov 4(%bp),%di  // s
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
# ifdef __IA16_CALLCVT_STDCALL
	ret $2
# else
	ret
# endif
#else
	mov %es, %dx
	mov %di, %bx

	mov %ds, %cx
	mov %cx, %es
	xchg %ax, %di
	mov $-1, %cx
	xor %al, %al

	cld
	repne
	scasb

	xchg %ax, %cx
	not %ax
	dec %ax

	mov %bx, %di
	mov %dx, %es
	ret
#endif

//------------------------------------------------------------------------------

