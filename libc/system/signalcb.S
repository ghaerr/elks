// Signal callback from kernel

#include <libc-private/call-cvt.h>

	.arch	i8086, nojumps
	.code16

	.text

	.global _signal_cbhandler

_signal_cbhandler:
	push %bp
	mov %sp,%bp

	pushf
	push %ax
	push %bx
	push %cx
	push %dx
	push %si
	push %di
	push %es

	mov 6(%bp),%bx
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %bx
#else
	mov %bx,%ax
#endif
#ifdef __IA16_CMODEL_IS_FAR_TEXT
	add %bx,%bx
	add %bx,%bx
	lcallw *_sigtable-4(%bx)  // offset by 4 because no entry for signal 0
#else
	add %bx,%bx
	mov	_sigtable-2(%bx),%bx  // offset by 2 because no entry for signal 0
	call *%bx
#endif
#ifdef __IA16_CALLCVT_CDECL
	inc %sp
	inc %sp
#endif

	pop %es
	pop %di
	pop %si
	pop %dx
	pop %cx
	pop %bx
	pop %ax
	popf

	pop %bp
	lret $2  // get rid of the signum

	.data
	.extern _sigtable
