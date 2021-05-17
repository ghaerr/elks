// Fast custom serial COM interrupt routine for ELKS
//
// runs on any stack and skips all ELKS overhead
// must run with interrupts disabled as could interrupt user, kernel or interrupt stack
// reads character into ring buffer
// timer interrupt runs rs_pump() which checks for non-zero queue and calls wake_up
//
// 25 June 2020 Greg Haerr
//
#include <linuxmt/config.h>
#include <arch/ports.h>
	.code16
	.text

#ifdef CONFIG_FAST_IRQ4
//
// fast com1 interrupt routine, uses "jmp _irq_com1" from interrupt vector
//
	.extern	fast_com1_irq
	.global	_irq_com1
_irq_com1:
	push	%ax			// save regs, uses 18 bytes of current stack
	push	%bx
	push	%cx
	push	%dx
	push	%ds

	// Recover kernel data segment
	// Was pushed by the CALLF of the dynamic handler
	mov	%sp,%bx
	mov	%ss:12(%bx),%ds

	call	fast_com1_irq		// call special C interrupt routine
					// which doesn't use any SS/SP/BP addressing

	mov	$0x20,%al		// EOI on primary controller
	out	%al,$0x20

	pop	%ds			// restore regs
	pop	%dx
	pop	%cx
	pop	%bx
	pop	%ax
	add	$4,%sp		// skip the trampoline DS:*irq
	iret
#endif

#ifdef CONFIG_FAST_IRQ3
//
// fast com2 interrupt routine, uses "jmp _irq_com2" from interrupt vector
//
	.extern	fast_com2_irq
	.global	_irq_com2
_irq_com2:
	push	%ax			// save regs, uses 18 bytes of current stack
	push	%bx
	push	%cx
	push	%dx
	push	%ds

	// Recover kernel data segment
	// Was pushed by the CALLF of the dynamic handler
	mov	%sp,%bx
	mov	%ss:12(%bx),%ds

	call	fast_com2_irq		// call special C interrupt routine
					// which doesn't use any SS/SP/BP addressing

	mov	$0x20,%al		// EOI on primary controller
	out	%al,$0x20

	pop	%ds			// restore regs
	pop	%dx
	pop	%cx
	pop	%bx
	pop	%ax
	add	$4,%sp		// skip the trampoline DS:*irq
	iret
#endif
