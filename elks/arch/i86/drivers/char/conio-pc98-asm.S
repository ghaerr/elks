// CONIO API to BIOS for PC98 Console
// @tyama501 Dec 2021
//
	.arch	i8086, nojumps
	.code16
	.text

	.global early_putchar
	.global bios_getchar
	.global bios_getarrow
	.global bios_getinit
	.global read_tvram_x
	.global write_tvram_x
	.global clear_tvram
	.global cursor_on
	.global cursor_off
	.global cursor_set

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

bios_getchar:
	push %bx
	mov $5,%ah
	int $0x18
	cmp $0x01,%bh
	jz get_end
	xor %ax,%ax
get_end:
	pop %bx
	ret

bios_getarrow:
	mov $0x0407,%ax
	int $0x18
	ret

bios_getinit:
	mov $3,%ah
	int $0x18
	ret

cursor_on:
	mov $17,%ah
	int $0x18
	ret

cursor_off:
	mov $18,%ah
	int $0x18
	ret

cursor_set:
	mov %sp,%bx
	mov 2(%bx),%dx
	mov $19,%ah
	int $0x18
	ret

tvram_x:
	.word 480
