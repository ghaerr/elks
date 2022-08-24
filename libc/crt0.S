// C runtime bootstrap
// This must be the first module of the executable

#include <libc-private/call-cvt.h>

	.code16

	.text

	.extern main
	.extern _exit

// This is the program entry point

	.global _start

_start:

// C runtime startup
// Stack is empty and immediately followed by argc, argv and envp

	push %bp
	mov %sp,%bp
	lea 4(%bp),%bx  // argv [0]
	mov 2(%bp),%cx  // argc
	mov %cx,%ax
	inc %ax
	shl $1,%ax
	add %bx,%ax     // envp [0]
	mov %ax,environ
	push %ax
	push %bx
	mov (%bx),%bx
	mov %bx,_program_filename
	push %cx
	CALL_(main)
#ifdef __IA16_CALLCVT_CDECL
	add $6,%sp
#endif
	push %ax  // main return value
	CALL_N_(exit)  // no return
	int $3

	.global exit

exit:
	push %bp
	mov %sp,%bp
#if defined __MEDIUM__ || defined __LARGE__ || defined __HUGE__
	mov $_cleanup,%si
	mov (%si),%bx
	or 2(%si),%bx
	jz _exit_end
	lcallw *(%si)
#else
	mov _cleanup,%bx
	or %bx,%bx
	jz _exit_end
	call *%bx
#endif

_exit_end:
	push 4+FAR_ADJ_(%bp)  // exit code
	CALL_(_exit)  // kernel one - no return
	int $3

	.data

	.extern environ
	.extern _program_filename
	.extern _cleanup

// Zero data for null pointers (near & far)
// Will be linked as first section in data segment

	.section .nildata

	.word 0
	.word 0
