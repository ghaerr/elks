####################################################################################
# BIOS INT 1FH Block Move and A20 enable for PC-98
#
	.arch	i8086, nojumps
	.code16
	.text

	.global	block_move
	.global	enable_a20_gate
	.global	verify_a20

# A20 gate functions shared with setup.S
#include "a20-pc98.inc"

#
# int block_move(struct gdt_table *gdtp, size_t words)
# Uses BIOS INT 1Fh AH=90h Block Move
# ES:BX gdtp
# CX    byte count
# SI    0000h
# DI    0000h
#
block_move:
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
	jnc	1f
	sti			# ensure we've got interrupts
	mov	$-1,%ax		# fail return AX < 0
	jmp	2f
1:	xor	%ax,%ax		# success return AX = 0
2:	pop	%bp
	pop	%di
	pop	%si
	pop	%es
	ret
