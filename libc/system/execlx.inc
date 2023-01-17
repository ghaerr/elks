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

/* Adapted from spawnlx.S in libi86. */

#include <libc-private/call-cvt.h>

	.arch	i8086, nojumps
	.code16

	.text

#if defined L_execl
	.global execl
execl:
#elif defined L_execle
	.global	execle
execle:
#elif defined L_execlp
	.global	execlp
execlp:
#elif defined L_execlpe
	.global	execlpe
execlpe:
#else
# error "which function are we assembling?"
#endif
#if defined __IA16_CALLCVT_REGPARMCALL
	push %ds
	push %es
	push %si
	push %di
	xchg %ax,%bx			/* remember bx = fname */
	push %ds
	pop %es
	/*
	 * sp:			-> sp:
	 *	saved di		saved di
	 *	saved si		saved si
	 *	saved es		saved es
	 *	...2-byte slot...	saved caller ip
	 *	saved caller ip		[saved caller cs]
	 *	[saved caller cs]	arg0
	 *	arg1			arg1
	 *	...			...
	 */
	mov %sp,%di
	add $6,%di
	lea 2(%di),%si
	movsw
# ifdef __IA16_CMODEL_IS_FAR_TEXT
	movsw
# endif
	mov %di,%ax
	xchg %ax,%dx			/* set dx = argv; store arg0 */
	stosw
# if defined L_execle || defined L_execlpe
	/*
	 * Locate envp: scan the argv[] array for the final NULL.  Handle
	 * the (rare) case where arg0 = NULL & so argv[] = { NULL }.  The
	 * 0x3c "eats up" the `lodsw' (0xad) opcode for the first loop
	 * iteration & turns it into `cmp $0xad,%al' (0x3c 0xad).
	 */
	.byte 0x3c
.find_envp:
	lodsw
	test %ax,%ax
	jnz .find_envp
	mov (%si),%cx			/* set cx = envp */
# else
	mov environ,%cx
# endif
	xchg %ax,%bx			/* set ax = fname */
	pop %di
	pop %si
	pop %es
# if defined L_execlp || defined L_execlpe
	CALL_(execvpe)
# else
	CALL_(execve)
# endif
# ifdef __IA16_CMODEL_IS_FAR_TEXT
	lret $2
# else
	ret $2
# endif
#elif defined __IA16_CALLCVT_STDCALL
	push %ds
	push %ds
	push %es
	push %si
	push %di
	push %ds
	pop %es
	/*
	 * sp:			-> sp:
	 *	saved di		saved di
	 *	saved si		saved si
	 *	saved es		saved es
	 *	...4-byte slot...	saved caller ip
	 *				[saved caller cs]
	 *	saved caller ip		fname
	 *	[saved caller cs]	argv
	 *	fname			envp
	 *	arg0			arg0
	 *	...			...
	 */
	mov %sp,%di
	add $6,%di
	lea 4(%di),%si
	movsw				/* move caller ip */
# ifdef __IA16_CMODEL_IS_FAR_TEXT
	movsw				/* move caller cs */
# endif
	movsw				/* move fname */
	mov %si,%ax			/* store argv */
	stosw
# if defined L_execle || defined L_execlpe
.find_envp:
	lodsw
	test %ax,%ax
	jnz .find_envp
	movsw				/* move envp */
# else
	mov environ,%ax			/* store envp := environ */
	stosw
# endif
	pop %di
	pop %si
	pop %es
# if defined L_execlp || defined L_execlpe
	JMP_(execvpe)
# else
	JMP_(execve)
# endif
#elif defined __IA16_CALLCVT_CDECL
	sub $6,%sp
	push %es
	push %si
	push %di
	push %ds
	pop %es
	/*
	 * sp:			-> sp:
	 *	saved di		saved di
	 *	saved si		saved si
	 *	saved es		saved es
	 *	...6-byte slot...	fname
	 *				argv
	 *				envp
	 *	saved caller ip		saved caller ip
	 *	[saved caller cs]	[saved caller cs]
	 *	fname			fname
	 *	arg0			arg0
	 *	...			...
	 */
	mov %sp,%di
	add $6,%di
	lea 6+2+FAR_ADJ_(%di),%si
	movsw				/* move fname */
	mov %si,%ax			/* store argv */
	stosw
# if defined L_execle || defined L_execlpe
.find_envp:
	lodsw
	test %ax,%ax
	jnz .find_envp
	movsw				/* move envp */
# else
	mov environ,%ax			/* store envp := environ */
	stosw
# endif
	pop %di
	pop %si
	pop %es
# if defined L_execlp || defined L_execlpe
	CALL_(execvpe)
# else
	CALL_(execve)
# endif
	add $6,%sp
	RET_VA_
#else
# error "unknown calling convention!"
#endif
