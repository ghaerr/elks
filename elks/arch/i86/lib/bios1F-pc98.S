####################################################################################
# BIOS INT 1FH Block Move and A20 enable for PC-98
#
	.arch	i8086, nojumps
	.code16
	.text

	.global	bios_block_movew
	.global	enable_a20_gate

# PC-98 A20 gate functions shared with setup.S
#include "a20-pc98.inc"

#
# void bios_block_movew(struct gdt_table *gdtp, size_t words)
# Uses BIOS INT 1Fh AH=90h Block Move
# NOTE: BIOS disables A20 during call, won't work with HMA kernel
# ES:BX gdtp
# CX    byte count
# SI    0000h
# DI    0000h
#
bios_block_movew:
	push	%es
	push	%si
	push	%di
	push	%bp
	mov	%sp,%bp

	xor	%si,%si
	xor	%di,%di

	mov	12(%bp),%cx	# word count
	shl	%cx		# byte count -> CX
	mov	10(%bp),%bx	# gdtp -> ES:BX
	push	%ds
	pop	%es

	mov	$0x90,%ah	# BIOS Block Move
	int	$0x1F
	sti			# re-enable interrupts from int15_fmemcpy
	pop	%bp
	pop	%di
	pop	%si
	pop	%es
	ret
