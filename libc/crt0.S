// C runtime bootstrap
// This must be the first module of the executable

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
	push %cx
	call main
#ifdef __IA16_CALLCVT_CDECL
	add $6,%sp
#endif
	push %ax  // main return value
	call exit  // no return
	int $3

	.global exit

exit:
	push %bp
	mov %sp,%bp
	mov _cleanup,%bx
	or %bx,%bx
	jz _exit_end
	call *%bx

_exit_end:
	push 4(%bp)  // exit code
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
