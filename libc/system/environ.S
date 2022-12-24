// Define and initialize the environ variable, if needed
// Assume ax = envp from entry point, from libc/crt0.S

	.arch i8086, nojumps
	.code16

	.section .preinit,"ax",@progbits

	mov %ax,environ

//------------------------------------------------------------------------------

	.comm environ,2
