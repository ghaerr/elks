// Define and initialize the __stacklow variable, if needed
// Assume dx = stacksize from entry point, from libc/crt0.S

	.arch i8086, nojumps
	.code16

	.section .preinit,"ax",@progbits

	mov %sp,%si         // SP will be 4 too high if __argc or __argv used
	sub %dx,%si
	mov %si,__stacklow

//------------------------------------------------------------------------------

	.comm __stacklow,2
