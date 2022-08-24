/* ia16-elf-gcc ASM instrumentation functions for ELKS */
/* June 2022 Greg Haerr */

	.code16
	.text

	.global print_regs
	.global print_segs
	.extern	printf

        .global getcsbyte
// int getcsbyte(char *addr)
// return byte at CS:addr
getcsbyte:
        push %bp
        mov  %sp,%bp
        mov  4(%bp),%bx
        mov  %cs:(%bx),%al
        xor  %ah,%ah
        pop  %bp
        ret

        .global getcs
// int getcs(void)
// return CS
getcs:
        push %cs
        pop  %ax
        ret

        .global rdtsc
// unsigned long long rdtsc(void)
// 386+ only, reads CPU time stamp counter
rdtsc:
        mov  %sp,%bx
        mov  2(%bx),%bx // get address for 64-bit return
        rdtsc           // returns 64-bit EDX:EAX
        mov  %ax,(%bx)
        shr  $16,%eax
        mov  %ax,2(%bx)
        mov  %dx,4(%bx)
        shr  $16,%edx
        mov  %dx,6(%bx)
        ret

print_regs:
	push %ax
	push %sp
	push %bp
	mov  %sp,%bp
	add  $4,2(%bp)	// adjust SP to before call here

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

print_segs:
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
