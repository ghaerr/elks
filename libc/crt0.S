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

	pop %bp		// argc
	mov %sp,%si	// argv [0]
	lea 1(%bp),%di
	shl $1,%di
	add %si,%di	// envp [0]
	mov %di,environ
	mov (%si),%ax
	mov %ax,_program_filename

// ...Code fragments from .init & .init.* sections will go here...

	.section .inittail,"ax",@progbits

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

	.section .finihead,"ax",@progbits

	.global exit

exit:
#ifdef __IA16_CALLCVT_REGPARMCALL
	xchg %ax,%di
#endif

// ...Code fragments from .fini & .fini.* sections will go here...

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
