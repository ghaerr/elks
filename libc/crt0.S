// C runtime bootstrap
// This must be the first module of the executable

#include <libc-private/call-cvt.h>

	.arch	i8086, nojumps
	.code16

	.section .inithead,"ax",@progbits

	.extern main
	.extern _exit

// Ensure that constructor & destructor handling is linked in
// TODO: make these optional

	.reloc .,R_386_NONE,__CTOR_LIST__
	.reloc .,R_386_NONE,__DTOR_LIST__

// This is the program entry point

	.global _start

_start:

// C runtime startup
// Stack is empty and immediately followed by argc, argv and envp

	pop %cx		// argc
	mov %sp,%bx	// argv [0]
	push %cx	// push argc back so that ps can see it
	push %bp	// bp is zero, push stack trace terminator
	mov %sp,%bp	// standard function prologue for debugging
	mov %cx,%ax
	inc %ax
	shl $1,%ax
	add %bx,%ax	// envp [0]
	mov %ax,environ
	push %ax
	push %bx
	mov (%bx),%bx
	mov %bx,_program_filename
	push %cx

// ...Code fragments from .init & .init.* sections will go here...
//    NOTE: these are allowed to clobber ax, bx, cx, dx, si

	.section .inittail,"ax",@progbits

#ifdef __IA16_CALLCVT_REGPARMCALL
	pop %ax		// ax := argc
	pop %dx		// dx := argv
	pop %cx		// cx := envp
#endif
	CALL_(main)
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %ax  // main return value
#endif
	CALL_N_(exit)  // no return

	.section .finihead,"ax",@progbits

	.global exit

exit:
#ifdef __IA16_CALLCVT_REGPARMCALL
	xchg %ax,%di
#endif

// ...Code fragments from .fini & .fini.* sections will go here...
//    NOTE: these are allowed to clobber ax, bx, cx, dx, si

	.section .finitail,"ax",@progbits

#ifdef __IA16_CALLCVT_REGPARMCALL
	xchg %ax,%di
#endif
	JMP_(_exit)  // kernel one - no return

	.data

	.extern environ
	.extern _program_filename

// Zero data for null pointers (near & far)
// Will be linked as first section in data segment

	.section .nildata

	.word 0
	.word 0
