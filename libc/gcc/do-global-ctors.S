/*
 * Copyright (c) 2022 TK Chia
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 */

#include <libc-private/call-cvt.h>

	.code16

	.text

	.global	__do_global_ctors
__do_global_ctors:
	push	%si
	mov	$__CTOR_END__-2,%si
0:
#if FAR_ADJ_
	std
	lodsw
	lodsw
	cld
	inc	%ax
	jz	1f
	lcallw	*2(%si)
#else
	std
	lodsw
	cld
	inc	%ax
	jz	1f
	dec	%ax
	call	*%ax
#endif
	jmp	0b
1:
	pop	%si
	mov	$__do_global_dtors,%ax
#if defined __IA16_CALLCVT_CDECL
# if FAR_ADJ_
	push	%cs
# endif
	push	%ax
	CALL_(atexit)
	pop	%ax
# if FAR_ADJ_
	pop	%ax
# endif
	RET_(0)
#elif defined __IA16_CALLCVT_STDCALL
# if FAR_ADJ_
	push	%cs
# endif
	push	%ax
	CALL_(atexit)
	RET_(0)
#else
# if FAR_ADJ_
	mov	%cs,%dx
# endif
	JMP_(atexit)
#endif

	.section .data.__CTOR_LIST__,"aw",@progbits

	.global	__CTOR_LIST__
__CTOR_LIST__:
#if FAR_ADJ_
	.long	-1
#else
	.hword	-1
#endif

	.section .data.__CTOR_END__,"aw",@progbits

	.global	__CTOR_END__
__CTOR_END__:
#if FAR_ADJ_
	.long	0
#else
	.hword	0
#endif
