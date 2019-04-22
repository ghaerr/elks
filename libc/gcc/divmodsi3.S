
	.code16

	.text
	.extern	_ldivmod
	.extern	_ludivmod

	.global	__divsi3
	.align 1

__divsi3:
	push	%bp
	mov	%sp,%bp
	push	%bx
	push	%cx
	push	%di
#ifndef __IA16_CALLCVT_REGPARMCALL
	mov	4(%bp),%ax
	mov	6(%bp),%bx
	mov	8(%bp),%cx
	mov	10(%bp),%di
#else
	mov	%dx,%bx
	mov	4(%bp),%cx
	mov	6(%bp),%di
#endif
	call	_ldivmod
	mov	%di,%dx
	mov	%cx,%ax
	pop	%di
	pop	%cx
	pop	%bx
	pop	%bp
#ifdef __IA16_CALLCVT_STDCALL
	ret	$8
#elif defined __IA16_CALLCVT_REGPARMCALL
	ret	$4
#else
	ret
#endif

	.global	__modsi3
	.align 1

__modsi3:
	push	%bp
	mov	%sp,%bp
	push	%bx
	push	%cx
	push	%di
#ifndef __IA16_CALLCVT_REGPARMCALL
	mov	4(%bp),%ax
	mov	6(%bp),%bx
	mov	8(%bp),%cx
	mov	10(%bp),%di
#else
	mov	%dx,%bx
	mov	4(%bp),%cx
	mov	6(%bp),%di
#endif
	call	_ldivmod
	mov	%bx,%dx
	pop	%di
	pop	%cx
	pop	%bx
	pop	%bp
#ifdef __IA16_CALLCVT_STDCALL
	ret	$8
#elif defined __IA16_CALLCVT_REGPARMCALL
	ret	$4
#else
	ret
#endif

	.global	__udivsi3
	.align 1

__udivsi3:
	push	%bp
	mov	%sp,%bp
	push	%bx
	push	%cx
	push	%di
#ifndef __IA16_CALLCVT_REGPARMCALL
	mov	4(%bp),%ax
	mov	6(%bp),%bx
	mov	8(%bp),%cx
	mov	10(%bp),%di
#else
	mov	%dx,%bx
	mov	4(%bp),%cx
	mov	6(%bp),%di
#endif
	call	_ludivmod
	mov	%di,%dx
	mov	%cx,%ax
	pop	%di
	pop	%cx
	pop	%bx
	pop	%bp
#ifdef __IA16_CALLCVT_STDCALL
	ret	$8
#elif defined __IA16_CALLCVT_REGPARMCALL
	ret	$4
#else
	ret
#endif

	.global	__umodsi3
	.align 1

__umodsi3:
	push	%bp
	mov	%sp,%bp
	push	%bx
	push	%cx
	push	%di
#ifndef __IA16_CALLCVT_REGPARMCALL
	mov	4(%bp),%ax
	mov	6(%bp),%bx
	mov	8(%bp),%cx
	mov	10(%bp),%di
#else
	mov	%dx,%bx
	mov	4(%bp),%cx
	mov	6(%bp),%di
#endif
	call	_ludivmod
	mov	%bx,%dx
	pop	%di
	pop	%cx
	pop	%bx
	pop	%bp
#ifdef __IA16_CALLCVT_STDCALL
	ret	$8
#elif defined __IA16_CALLCVT_REGPARMCALL
	ret	$4
#else
	ret
#endif
