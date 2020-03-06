//------------------------------------------------------------------------------
// #include <string.h>
// void * memcpy (void * dest, const void * src, size_t n);
//------------------------------------------------------------------------------

	.code16

	.text

	.global memcpy

memcpy:
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
#ifdef __IA16_CALLCVT_CDECL
	ret
#elif defined __IA16_CALLCVT_STDCALL
	ret $6
#else
#error "unknown calling convention"
#endif

//------------------------------------------------------------------------------
