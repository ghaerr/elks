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

	.arch	i8086, nojumps
	.code16

	.section .fini,"ax",@progbits

	push	%si
	mov	$__DTOR_LIST__+2+FAR_ADJ_,%si
0:
#ifdef __IA16_CMODEL_IS_FAR_TEXT
	lodsw
	xchg	%ax,%cx
	lodsw
	or	%cx,%ax
	jz	1f
	lcallw	*-4(%si)
#else
	lodsw
	xchg	%ax,%cx
	jcxz	1f
	call	*%cx
#endif
	jmp	0b
1:
	pop	%si

	.section .data.__DTOR_LIST__,"aw",@progbits

	.balign	2
	.global	__DTOR_LIST__
__DTOR_LIST__:
#ifdef __IA16_CMODEL_IS_FAR_TEXT
	.long	0
#else
	.hword	0
#endif

	.section .data.__DTOR_END__,"aw",@progbits

	.global	__DTOR_END__
__DTOR_END__:
#ifdef __IA16_CMODEL_IS_FAR_TEXT
	.long	0
#else
	.hword	0
#endif
