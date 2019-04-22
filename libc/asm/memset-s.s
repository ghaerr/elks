//------------------------------------------------------------------------------
// #include <string.h>
// void * memset (void * s, int c, size_t n);
//------------------------------------------------------------------------------

	.code16

	.text

	.global memset

memset:
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

	pop %bp
	ret

//------------------------------------------------------------------------------
