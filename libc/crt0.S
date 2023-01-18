// C runtime bootstrap
// This must be the first module of the executable

#include <sys/linksym.h>
#include <libc-private/call-cvt.h>

	.arch	i8086, nojumps
	.code16

// Requires the following ordered sections in linker scripts:
//  .inithead   - _startup: envp, argv and argc computed and pushed
//  .preinit    - initialize any referenced .comm variables
//  .init       - run constructor routines
//  .postinit   - restore stack if __argv, __argc used
//  .inittail   - main called, converting stack if regparmcall
//  .finihead   - exit called
//  .fini       - run atexit routines then other destructors
//  .finitail   - _exit(retcode) called

	.section .inithead,"ax",@progbits

	.extern main
	.extern _exit

// Ensure that constructor & destructor handling is linked in
// TODO: make these optional

	__LINK_SYMBOL(__CTOR_LIST__)
	__LINK_SYMBOL(__DTOR_LIST__)

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
	push %ax        // envp
	push %bx        // argv
	push %cx        // argc

// ...Code fragments from .preinit & .preinit.* sections will go here...
//    NOTE: these assume ax = envp, bx = argv, cx = argc & may clobber dx

// ...Then code fragments from .init & .init.* sections will go here...
//    NOTE: these are allowed to clobber ax, bx, cx, dx, si

// ...Then code fragments from .postinit & .postinit.* sections will go here...
//    NOTE: these are allowed to clobber ax, bx, cx, dx, si

	.section .inittail,"ax",@progbits

#ifdef __IA16_CALLCVT_REGPARMCALL
	pop %ax		// ax := argc
	pop %dx		// dx := argv
	pop %cx		// cx := envp
#endif
	CALL_(main)
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %ax        // main return value
#endif
	CALL_N_(exit)   // no return

	.section .finihead,"ax",@progbits

	.global exit

exit:
#ifdef __IA16_CALLCVT_REGPARMCALL
	xchg %ax,%di    // save exit arg in DI
#endif

// ...Code fragments from .fini & .fini.* sections will go here...
//    NOTE: these are allowed to clobber ax, bx, cx, dx, si

	.section .finitail,"ax",@progbits

#ifdef __IA16_CALLCVT_REGPARMCALL
	xchg %ax,%di    // retrieve exit arg
#endif
	JMP_(_exit)     // no return

	.data

// Zero data for null pointers (near & far)
// Will be linked as first section in data segment

	.section .nildata

	.word 0
	.word 0
