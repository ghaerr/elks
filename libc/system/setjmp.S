#include <libc-private/call-cvt.h>

	.code16

	.text

	.global _setjmp

_setjmp:

	pop %cx  // PC
#if defined __MEDIUM__ || defined __LARGE__ || defined __HUGE__
	pop %dx  // CS
#endif
#ifdef __IA16_CALLCVT_CDECL
	mov %sp,%bx
	mov (%bx),%bx  // TOS is prt -> env
#elif defined __IA16_CALLCVT_STDCALL
	pop %bx
#else
#error "unknown calling convention"
#endif

	mov %cx,0(%bx)  // PC
#if defined __MEDIUM__ || defined __LARGE__ || defined __HUGE__
	mov %dx,2(%bx)  // CS
#endif
	mov %sp,2+FAR_ADJ_(%bx) // This registers are all that may be constant.
	mov %bp,4+FAR_ADJ_(%bx)
	mov %si,6+FAR_ADJ_(%bx) // SI, DI & ES must be saved for GCC
	mov %di,8+FAR_ADJ_(%bx)
	mov %es,10+FAR_ADJ_(%bx)

	xor %ax,%ax
#if defined __MEDIUM__ || defined __LARGE__ || defined __HUGE__
	push %dx
	push %cx
	lret
#else
	jmp *%cx
#endif


	.global _longjmp

_longjmp:

	pop %cx  // PC
#if defined __MEDIUM__ || defined __LARGE__ || defined __HUGE__
	pop %dx  // CS
#endif
	pop %bx  // env->
	pop %ax  // rv

	mov 0(%bx),%cx  // PC
#if defined __MEDIUM__ || defined __LARGE__ || defined __HUGE__
	mov 2(%bx),%dx  // CS
#endif
	mov 2+FAR_ADJ_(%bx),%sp
	mov 4+FAR_ADJ_(%bx),%bp
	mov 6+FAR_ADJ_(%bx),%si
	mov 8+FAR_ADJ_(%bx),%di
	mov 10+FAR_ADJ_(%bx),%es

#if defined __MEDIUM__ || defined __LARGE__ || defined __HUGE__
	push %dx
	push %cx
	lret
#else
	jmp *%cx
#endif
