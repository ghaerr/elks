/* ia16-elf-gcc ASM instrumentation functions for ELKS */
/* June 2022 Greg Haerr */
	.code16
	.text

	.global _print_regs
	.global _print_segs
	.extern	printf

_print_regs:
	push %ax
	push %sp
	push %bp
	mov  %sp,%bp
	addw $4,2(%bp)	// adjust SP to before call here

	push %di
	push %si
	push %dx
	push %cx
	push %bx
	push %ax
	mov  $fmt_regs,%ax
	push %ax
	call printf
	pop %ax

	pop %ax
	pop %bx
	pop %cx
	pop %dx
	pop %si
	pop %di
	pop %bp
	pop %ax		// don't reset SP
	pop %ax
	//ret

_print_segs:
	push %ax
	push %bx
	push %cx
	push %dx
	push %bp

	mov %sp,%bp
	mov %bp,%ax
	add $12,%ax     // adjust SP to before call here
	push %ax
	push %ss
	push %es
	push %ds
	push 10(%bp)    // orig IP
	push %cs
	mov $fmt_sregs,%ax
	push %ax
	call printf
	add $14,%sp

	pop %bp
	pop %dx
	pop %cx
	pop %bx
	pop %ax
	ret

	.data
fmt_regs: .ascii "AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x BP=%04x SP=%04x\n"
	.byte	0
fmt_sregs: .ascii "CS:IP=%04x:%04x DS=%04x ES=%04x SS:SP=%04x:%04x\n"
	.byte	0
