// enable the autoconfig tool

#define __ASSEMBLY__

#include <linuxmt/config.h>
#include <arch/asm-offsets.h>

//	Assembler boot strap hooks. This is called by setup

	.arch i8086, nojumps
	.code16
	.text

	.global _start
	.extern	start_kernel
	.global early_putchar
#ifdef CONFIG_ARCH_PC98
	.global early_getchar
	.global early_getinit
	.global read_tvram_x
	.global write_tvram_x
	.global clear_tvram
#endif

_start:

/*
! Setup.S already initialized DS and ES (but not SS)
! In addition, registers contain:
!   BX, Text size
!   DI  Far text size
!   SI, Data size
!   DX, BSS size
*/
	mov	%bx,_endtext
	mov	%di,_endftext
	mov	%si,_enddata
	add	%dx,%si
	mov	%si,_endbss

// Start cleaning BSS. Still using setup.S stack

	mov	_enddata,%di	// start of BSS
	mov	%dx,%cx		// CX = BSS size
	xor	%ax,%ax
	shr	$1,%cx
	cld
	rep
	stosw

// End cleaning BSS

	mov	%cs,kernel_cs
	mov	%ds,kernel_ds

// Set SS:SP to task[0] kernel stack area

	mov	%ds,%ax
	mov	%ax,%ss		// SS=ES=DS
	mov	$task + TASK_USER_AX,%sp

	call	start_kernel	// fall through into breakpoint if returns

	.global int3
int3:	int	$3		// C breakpoint for emu86
	ret

#ifdef CONFIG_ARCH_PC98
early_putchar:
	mov %sp,%bx
	mov 2(%bx),%al
	push %si
	push %ds
	push %bx
	push %di
	push %es
	push %dx
	push %cs
	pop %ds
	cmp $'\r',%al
	jz putesc_r
	cmp $'\n',%al
	jz putesc_n
	cmp $'\b',%al
	jz putesc_b
	xor %ah,%ah
	mov $tvram_x,%bx
	mov (%bx),%di
	mov $0xa000,%dx
	mov %dx,%es
	mov %ax,%es:(%di)
	inc %di
	inc %di
	mov %di,%ax
	sub $4000,%ax
	jnc put_end
	mov %di,(%bx)
put_end:
	pop %dx
	pop %es
	pop %di
	pop %bx
	pop %ds
	pop %si
	ret
putesc_r:
	mov $tvram_x,%bx
	mov (%bx),%ax
	mov $160,%dl
	div %dl
	mul %dl
	mov %ax,(%bx)
	jmp put_end
putesc_n:
	mov $tvram_x,%bx
	mov (%bx),%ax
	mov $160,%dl
	div %dl
	inc %al
	mul %dl
	mov %ax,%dx
	sub $4000,%ax
	jnc put_scroll
	mov %dx,(%bx)
	jmp put_end
putesc_b:
	mov $tvram_x,%bx
	mov (%bx),%ax
	cmp $0,%ax
	jz put_end
	dec %ax
	dec %ax
	mov %ax,(%bx)
	jmp put_end
put_scroll:
	xor %ax,%ax
	mov $24,%bx
	mov $0xa000,%dx
	mov %dx,%es
	mov %dx,%ds
	mov %ax,%di
	add $160,%ax
	mov %ax,%si
scroll_lp:
	mov $80,%cx
	cld
	rep
	movsw
	dec %bx
	jnz scroll_lp
	mov $80,%cx
	xor %ax,%ax
	mov $3840,%dx
	mov %dx,%di
	rep
	stosw
	mov %dx,tvram_x
	jmp put_end
read_tvram_x:
	push %ds
	push %cs
	pop %ds
	mov tvram_x,%ax
	pop %ds
	ret
write_tvram_x:
	mov %sp,%bx
	mov 2(%bx),%ax
	push %ds
	push %cs
	pop %ds
	mov %ax,tvram_x
	pop %ds
	ret
clear_tvram:
	push %bx
	push %dx
	push %ds
	push %es
	push %cs
	pop %ds
	mov tvram_x,%bx
	mov $0xa000,%dx
	mov %dx,%es
	movw $0x0000,%es:(%bx)
	pop %es
	pop %ds
	pop %dx
	pop %bx
	ret
tvram_x:
	.word 960
#else
early_putchar:
	mov   %sp,%bx
	mov   2(%bx),%al
	mov   $0x0E,%ah
	mov   $0x0007,%bx
	int   $0x10
	ret
#endif
#ifdef CONFIG_ARCH_PC98
early_getchar:
	push %bx
	mov $5,%ah
	int $0x18
	cmp $0x01,%bh
	jz get_end
	xor %ax,%ax
get_end:
	pop %bx
	ret
early_getinit:
	mov $3,%ah
	int $0x18
	ret
#endif

//	Segment beginnings

// Zero for NULL pointers (near and far)
// Will be linked as first section in data segment
	.section .nildata
	.word 	0
	.word	0

	.data
	.global _endtext
	.global _endftext
	.global _enddata
	.global _endbss
	.extern	kernel_cs
	.extern	kernel_ds
	.extern task

_endtext:
	.word	0

_endftext:
	.word	0

_enddata:
	.word	0

_endbss:
	.word	0
	.bss

_sbss:
