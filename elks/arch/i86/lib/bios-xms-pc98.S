####################################################################################
# BIOS INT 1FH Block Move and A20 enable for PC-98

	.arch	i8086, nojumps
	.code16
	.text

	.global	block_move
	.global	enable_a20_gate
	.global	verify_a20

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

# Attempt to enable A20 address gate, return 0 on fail
enable_a20_gate:
	mov	$0,%al
	out	%al,$0xF2
	mov	$2,%al
	out	%al,$0xF6
	call	verify_a20	# returns 1 if enabled, 0 if disabled
	ret

# verify if A20 gate is enabled, return 0 if disabled
verify_a20:
	push	%ds
	push	%es

	xor	%ax,%ax
	mov	%ax,%ds
	dec	%ax
	mov	%ax,%es

	pushf			# save interrupt status
	cli			# interrupts off

	mov	%es:0x10,%ax	# read word at FFFF:0010 (1 meg)
	not	%ax		# 1's complement
	pushw	0		# save word at 0000:0000 (0)

	mov	%ax,0		# word at 0 = ~(word at 1 meg)
	mov	0,%ax		# read it back
	cmp	%es:0x10,%ax	# fail if word at 0 == word at 1 meg

	popw	0

	jz	1f		# if ZF=1, the A20 gate is NOT enabled
	mov	$1,%ax		# return 1 if enabled
9:	popf			# restore interrupt status
	pop	%es
	pop	%ds
	ret
1:	mov	$0,%ax		# return 0 if disabled
	jmp	9b
