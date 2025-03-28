####################################################################################
# A20 enable for PC-98
#
# This file is #included into bios-xms-pc98.S and setup.S
#
	.global	enable_a20_gate
	.global	verify_a20

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
