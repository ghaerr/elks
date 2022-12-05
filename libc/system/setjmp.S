#include <libc-private/call-cvt.h>

	.arch	i8086, nojumps
	.code16

	.text

	.global _setjmp

_setjmp:

	pop %cx  // PC
#ifdef __IA16_CMODEL_IS_FAR_TEXT
	pop %dx  // CS
#endif
#if defined __IA16_CALLCVT_CDECL
	mov %sp,%bx
	mov (%bx),%bx  // TOS is prt -> env
#elif defined __IA16_CALLCVT_STDCALL
	pop %bx
#elif defined __IA16_CALLCVT_REGPARMCALL
	xchg %ax,%bx
#else
#error "unknown calling convention"
#endif

	mov %cx,0(%bx)  // PC
#ifdef __IA16_CMODEL_IS_FAR_TEXT
	mov %dx,2(%bx)  // CS
#endif
	mov %sp,2+FAR_ADJ_(%bx) // This registers are all that may be constant.
	mov %bp,4+FAR_ADJ_(%bx)
	mov %si,6+FAR_ADJ_(%bx) // SI, DI & ES must be saved for GCC
	mov %di,8+FAR_ADJ_(%bx)
	mov %es,10+FAR_ADJ_(%bx)

	xor %ax,%ax
#ifdef __IA16_CMODEL_IS_FAR_TEXT
	push %dx
	push %cx
	lret
#else
	jmp *%cx
#endif


	.global _longjmp

_longjmp:

	pop %cx  // PC
#ifdef __IA16_CMODEL_IS_FAR_TEXT
	pop %dx  // CS
#endif
#ifndef __IA16_CALLCVT_REGPARMCALL
	pop %bx  // env->
	pop %ax  // rv
#else
	xchg %ax,%bx  // env->
	xchg %ax,%dx  // rv
#endif

	mov 0(%bx),%cx  // PC
#ifdef __IA16_CMODEL_IS_FAR_TEXT
	mov 2(%bx),%dx  // CS
#endif
	mov 2+FAR_ADJ_(%bx),%sp
	mov 4+FAR_ADJ_(%bx),%bp
	mov 6+FAR_ADJ_(%bx),%si
	mov 8+FAR_ADJ_(%bx),%di
	mov 10+FAR_ADJ_(%bx),%es

#ifdef __IA16_CMODEL_IS_FAR_TEXT
	push %dx
	push %cx
	lret
#else
	jmp *%cx
#endif
