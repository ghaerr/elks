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

	pop %bp		// argc
	mov %sp,%si	// argv [0]
	lea 1(%bp),%di
	shl $1,%di
	add %si,%di	// envp [0]
	mov %di,environ
	mov (%si),%ax
	mov %ax,_program_filename
	CALL_N_(__do_global_ctors)
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %di
	push %si
	push %bp
#else
	xchg %ax,%bp	// ax := argc
	mov %si,%dx	// dx := argv
	mov %di,%cx	// cx := envp
#endif
	CALL_(main)
#ifdef __IA16_CALLCVT_CDECL
	add $6,%sp
#endif
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %ax  // main return value
#endif
	CALL_N_(exit)  // no return
	int $3

	.global exit

exit:
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %bp
	mov %sp,%bp
#else
	xchg %ax,%di
#endif
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
#ifndef __IA16_CALLCVT_REGPARMCALL
	push 4+FAR_ADJ_(%bp)  // exit code
#else
	xchg %ax,%di
#endif
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
