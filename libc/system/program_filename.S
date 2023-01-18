// Define and initialize the __program_filename variable, if needed
// Assume bx = argv from entry point, from libc/crt0.S

	.arch i8086, nojumps
	.code16

	.section .preinit,"ax",@progbits

	mov (%bx),%dx
	mov %dx,__program_filename

//------------------------------------------------------------------------------

	.comm __program_filename,2
