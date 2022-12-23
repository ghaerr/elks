// Define and initialize the _program_filename variable, if needed
// Assume bx = argv from entry point, from libc/crt0.S

	.arch i8086, nojumps
	.code16

	.section .preinit,"ax",@progbits

	mov (%bx),%dx
	mov %dx,_program_filename

//------------------------------------------------------------------------------

	.comm _program_filename,2
