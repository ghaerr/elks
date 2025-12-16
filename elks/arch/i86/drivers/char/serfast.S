// First part of top half of fast serial interrupt handlers for ELKS
//
// Runs on any stack and skips ELKS _irqit stack switching overhead.
// Must run with interrupts disabled as could interrupt user, kernel or interrupt stack.
// Calls the second part of the top half (rs_fast_irq4) to read and queue received byte.
// The bottom half (serial_bh) runs later, which processes queue and calls wake_up.
//
// 25 June 2020 Greg Haerr
//
#include <linuxmt/config.h>
	.code16
	.text

#ifdef CONFIG_FAST_IRQ4
//
// Entry for ttyS0 top half interrupt handler, called by CALLF within dynamic handler
//
	.extern	rs_fast_irq4
	.global asm_fast_irq4
asm_fast_irq4:
	push	%ax			// save regs, uses 18 bytes of current stack
	push	%bx
	push	%cx
	push	%dx
	push	%ds

	// Recover kernel data segment
	// Was pushed by the CALLF of the dynamic handler
	mov	%sp,%bx
	mov	%ss:12(%bx),%ds

	call	rs_fast_irq4		// call special 2nd part of top half C handler
					// which doesn't use any SS/SP/BP addressing

	mov	$0x20,%al		// EOI on primary controller
	out	%al,$0x20

	pop	%ds			// restore regs
	pop	%dx
	pop	%cx
	pop	%bx
	pop	%ax
	add	$4,%sp			// skip the trampoline DS:*irq
	iret
#endif

#ifdef CONFIG_FAST_IRQ3
//
// Entry for ttyS1 top half interrupt handler, called by CALLF within dynamic handler
//
	.extern	rs_fast_irq3
	.global	asm_fast_irq3
asm_fast_irq3:
	push	%ax			// save regs, uses 18 bytes of current stack
	push	%bx
	push	%cx
	push	%dx
	push	%ds

	// Recover kernel data segment
	// Was pushed by the CALLF of the dynamic handler
	mov	%sp,%bx
	mov	%ss:12(%bx),%ds

	call	rs_fast_irq3		// call special 2nd part of top half C handler
					// which doesn't use any SS/SP/BP addressing

	mov	$0x20,%al		// EOI on primary controller
	out	%al,$0x20

	pop	%ds			// restore regs
	pop	%dx
	pop	%cx
	pop	%bx
	pop	%ax
	add	$4,%sp			// skip the trampoline DS:*irq
	iret
#endif

#ifdef CONFIG_FAST_IRQ1_NECV25
//
// Entry for console-serial-necv25 top half interrupt handler, called by CALLF within dynamic handler
//
    .extern	sercon_fast_irq1_necv25
    .global asm_fast_irq1_necv25
asm_fast_irq1_necv25:
    push    %ax         // save regs, uses 18 bytes of current stack
    push    %bx
    push    %cx
    push    %dx
    push    %ds

    // Recover kernel data segment
    // Was pushed by the CALLF of the dynamic handler
    mov	%sp,%bx
    mov	%ss:12(%bx),%ds

    call	sercon_fast_irq1_necv25 // call special 2nd part of top half C handler
                                    // which doesn't use any SS/SP/BP addressing

    .word   0x920f                  // NEC V25 specific FINT (End Of Interrupt) instruction

    pop     %ds			// restore regs
    pop     %dx
    pop     %cx
    pop     %bx
    pop     %ax
    add     $4,%sp      // skip the trampoline DS:*irq
    iret
#endif

#ifdef CONFIG_FAST_IRQ2_NECV25
//
// Entry for console-serial-necv25 top half interrupt handler, called by CALLF within dynamic handler
//
    .extern	sercon_fast_irq2_necv25
    .global asm_fast_irq2_necv25
asm_fast_irq2_necv25:
    push    %ax         // save regs, uses 18 bytes of current stack
    push    %bx
    push    %cx
    push    %dx
    push    %ds

    // Recover kernel data segment
    // Was pushed by the CALLF of the dynamic handler
    mov	%sp,%bx
    mov	%ss:12(%bx),%ds

    call	sercon_fast_irq2_necv25 // call special 2nd part of top half C handler
                                    // which doesn't use any SS/SP/BP addressing

    .word   0x920f                  // NEC V25 specific FINT (End Of Interrupt) instruction

    pop     %ds			// restore regs
    pop     %dx
    pop     %cx
    pop     %bx
    pop     %ax
    add     $4,%sp      // skip the trampoline DS:*irq
    iret
#endif
