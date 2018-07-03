//------------------------------------------------------------------------------
// #include <string.h>
// char * strcpy (char * dest, const char * src);
//------------------------------------------------------------------------------

	.code16

	.text

	.global strcpy

strcpy:
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
	ret

//------------------------------------------------------------------------------

