// Define and initialize __argc and __argv variables, if needed
// Assume argv and argc already pushed at entry point, from libc/crt0.S
// Requires .preinit and .postinit sections ordered in linker scripts

	.arch i8086, nojumps
	.code16

	.section .preinit,"ax",@progbits

	popw __argc	// load __argc, stack will be restored in .postinit
	popw __argv	// load __argv, ""

	.section .postinit,"ax",@progbits

	pushw __argv	// restore stack with possibly modified __argv, __argc
	pushw __argc

//------------------------------------------------------------------------------

	.comm __argc,2
	.comm __argv,2
