// Define and initialize __argc and __argv variables, if needed
// Assume argv and argc already pushed at entry point, from libc/crt0.S
// Requires .preinit and .postinit sections ordered in linker scripts

	.arch i8086, nojumps
	.code16

	.section .preinit,"ax",@progbits

	movw $__argc,%di
	popw (%di)	// load __argc, stack will be restored in .postinit
	popw 2(%di)	// load __argv, ""

	.section .postinit,"ax",@progbits

	pushw 2(%di)	// restore stack with possibly modified __argv, __argc
	pushw (%di)	// libc/crt0.S and .init routines should not clobber di

//------------------------------------------------------------------------------

	.bss
	.balign 2
	.global __argc
	.global __argv
__argc:
	.skip 2
__argv:			// place __argv close to __argc, so we can address
	.skip 2		// both compactly via a single base/index register
