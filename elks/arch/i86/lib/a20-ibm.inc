####################################################################################
# A20 line management routines for IBM PC
# derived from UNREAL.ASM and A20.ASM code by Chris Giese
# 8 Nov 2021 Greg Haerr
#
# int __far enable_a20_gate(void) - Enable A20 gate, return 0 on fail
# int verify_a20(void)            - Verify A20 gate status, return 0 if disabled
# void set_a20                    - ASM routine: enable (AH=1) or disable (AH=0) A20 gate
#
# This file is #included into unreal.S and setup.S

# configurable options for A20 gate
#   USE_BIOS		- use BIOS AX=2401h INT 15h method (always tried first)
#   USE_A20ASM		- use Chris Giese A20.ASM method (send D0, read, OR 2, D1, write)
#   USE_HIMEM_AT	- use himem.sys PC AT method (send D1,DF,FF)
#define USE_BIOS
#define USE_A20ASM
//#define USE_HIMEM_AT

# Attempt to enable A20 address gate, return 0 on fail
enable_a20_gate:
#ifdef USE_BIOS
	mov	$1,%ah		# enable A20
	call	bios_set_a20	# try BIOS first
	call	verify_a20	# returns 1 if enabled, 0 if disabled
	and	%ax,%ax
	jnz	1f
#endif
	mov	$1,%ah		# enable A20
	call	set_a20		# call configurable A20 handler
	call	verify_a20	# returns 1 if enabled, 0 if disabled
1:	lret

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

#ifdef USE_BIOS
#
# enable/disable A20 gate using keyboard port/controller, entry AH=0 disable
bios_set_a20:
	or	%ah,%ah
	jz	bios_reset_a20
	mov	$0x2401,%ax	# BIOS enable A20
	int	$0x15		# CF clear on success
	ret
bios_reset_a20:
	mov	$0x2400,%ax	# BIOS disable A20
	int	$0x15
	ret
#endif

#ifdef USE_A20ASM
set_a20:
	pushf			# save interrupt status
	cli			# interrupts off
	call	empty_8042
	mov	$0xD0,%al	# 8042 command byte to read output port
	out	%al,$0x64
1:	in	$0x64,%al
	test	$1,%al		# output buffer (data _from_ keyboard) full?
	jz	1b		# no, loop

	in	$0x60,%al	# read output port
	or	%ah,%ah
	jne	2f
	and	$0xFD,%al	# AND ~2 to disable
	jmp	3f
2:	or	$2,%al		# OR 2 to enable
3:	mov	%al,%ah

	call	empty_8042
	mov	$0xD1,%al	# 8042 command byte to write output port
	out	%al,$0x64

	call	empty_8042
	mov	%ah,%al		# the value to write
	out	%al,$0x60

	call	empty_8042
	popf			# restore interrupt status
	ret

0:	jmp	1f		# a delay (probably not effective nor necessary)
1:	jmp	2f
2:	in	$0x60,%al	# read and discard data/status from 8042
empty_8042:
	jmp	1f
1:	jmp	2f		# delay
2:	in	$0x64,%al
	test	$1,%al		# output buffer (data _from_ keyboard) full?
	jnz	0b		# yes, read and discard
	test	$2,%al		# input buffer (data _to_ keyboard) empty?
	jnz	empty_8042	# no, loop
	ret
#endif

#ifdef USE_HIMEM_AT
set_a20:
	pushf			# save interrupt status
	cli			# interrupts off
	xor	%ah,%ah
	jz	reset_a20

	call	sync_8042
	jnz	a20_err

	mov	$0xD1,%al	# send D1h (write output)
	out	%al,$0x64
	call	sync_8042
	jnz	a20_err

	mov	$0xDF,%al	# send DFh
	out	%al,$0x60
	call	sync_8042
	jnz	a20_err

	# wait for A20 line to settle down (up to 20 usecs)
	mov	$0xFF,%al	# send FFh (pulse output port NULL)
	out	%al,$0x64
	call	sync_8042
a20_err:
	popf			# restore interrupt status
	ret
reset_a20:
	call	sync_8042
	jnz	a20_err

	mov	$0xD1,%al	# send D1h (write output)
	out	%al,$0x64
	call	sync_8042
	jnz	a20_err

	mov	$0xDD,%al	# send DDh
	out	%al,$0x60
	call	sync_8042
	jnz	a20_err


	# wait for A20 line to settle down (up to 20 usecs)
	mov	$0xFF,%al	# send FFh (pulse output port NULL)
	out	%al,$0x64
	call	sync_8042
	popf			# restore interrupt status
	ret

sync_8042:
	xor	%cx,%cx
1:	in	$0x64,%al
	and	$2,%al
	loopnz	1b
	ret
#endif
