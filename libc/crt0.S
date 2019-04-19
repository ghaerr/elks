// C runtime bootstrap
// This must be the first module of the executable

	.code16

	.text

	.extern main
	.extern _exit

// This is the program entry point

	.global entry

entry:
	jmp _startup

// C runtime startup
// Stack is empty and immediately followed by argc, argv and envp

_startup:
	push %bp
	mov %sp,%bp
	lea 4(%bp),%bx  // argv [0]
	mov 2(%bp),%cx  // argc
	mov %cx,%ax
	shl $1,%ax
	add %bx,%ax     // envp [0]
	mov %ax,environ
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %ax
	push %bx
	push %cx
#else
	xchg %ax,%cx	// argc and envp
	mov %bx,%dx	// argv
#endif
	call main
	add $6,%sp
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %ax  // main return value
#endif
	call exit  // no return
	int $3

	.global exit

exit:
#ifdef __IA16_CALLCVT_REGPARMCALL
	xchg %ax,%si
#endif
	push %bp
	mov %sp,%bp
	mov _cleanup,%bx
	or %bx,%bx
	jz _exit_end
	call *%bx

_exit_end:
#ifndef __IA16_CALLCVT_REGPARMCALL
	push 4(%bp)  // exit code
#else
	xchg %ax,%si
#endif
	call _exit  // kernel one - no return
	int $3

	.data

	.extern environ
	.extern _cleanup

// Zero data for null pointers (near & far)
// Will be linked as first section in data segment

	.section .nildata

	.word 0
	.word 0
