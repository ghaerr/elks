
	.code16

	.text

	.global _setjmp

_setjmp:

	pop %cx  // PC
	mov %sp,%bx
	mov (%bx),%bx  // TOS is prt -> env

	mov %cx,0(%bx)  // PC
	mov %sp,2(%bx)  // This registers are all that may be constant.
	mov %bp,4(%bx)
	mov %si,6(%bx)  // SI, DI & ES must be saved for GCC
	mov %di,8(%bx)
	mov %es,10(%bx)

	xor %ax,%ax
	jmp *%cx


	.global _longjmp

_longjmp:

	pop %cx  // PC
	pop %bx  // env->
	pop %ax  // rv

	mov 0(%bx),%cx  // PC
	mov 2(%bx),%sp
	mov 4(%bx),%bp
	mov 6(%bx),%si
	mov 8(%bx),%di
	mov 10(%bx),%es

	jmp *%cx
