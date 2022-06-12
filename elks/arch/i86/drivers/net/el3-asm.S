//
// Low level routines for ELKS el3 ethernet driver (3c509)
//
// Helge Skrivervik (@mellvik) june 2022
//

#include <linuxmt/config.h>
#include "arch/ports.h"
#include <arch/asm-offsets.h>

	.code16

	.data
	.extern current


	.text

//-----------------------------------------------------------------------------
// Read data from port
//-----------------------------------------------------------------------------
// arg1: Port address (int)
// arg2: Data pointer (char *)
// arg3: Words to read (int)
//
// returns:
// Nothing

	.global el3_insw

el3_insw:

	cli
	push    %bp
	mov     %sp,%bp
	push    %di
	push	%es

	mov	4(%bp),%dx	// Port
	mov	6(%bp),%di	// Buffer pointer
	mov	8(%bp),%cx	// length (words)

        mov     current,%bx
        mov     TASK_USER_DS(%bx),%es	// destination segment = user process

word_loop:
        in      %dx,%ax
        stosw
        loop    word_loop

        //pop     %bx
        pop     %es
        pop     %di
	pop	%bp
	sti

        ret


//-----------------------------------------------------------------------------
// Write data to port
//-----------------------------------------------------------------------------
// arg1: Port address (int)
// arg2: Data pointer (char *)
// arg3: Word count (int)
//
// returns:
// Nothing

	.global el3_sendpk

el3_sendpk:
	cli
        push    %bp
        mov     %sp,%bp
        push    %si
	push	%ds

        mov     8(%bp),%cx      // word count
        mov     6(%bp),%si      // buffer addr
	mov	4(%bp),%dx	// Port

	mov	%cx,%ax
	outw	%ax,%dx		// send length
	xor	%ax,%ax
	outw	%ax,%dx		// dummy NULL word
	inc	%cx
	shr	%cx		// make word count
	inc	%cx
	and	$0xfe,%cx	// make dword aligned
	mov     current,%bx	// setup for far memory xfer
	mov     TASK_USER_DS(%bx),%ds

wr_loop:
	lodsw
	out     %ax,%dx
	loop	wr_loop

	pop     %ds
	pop	%si
	pop	%bp

	sti
	ret

//-------------------------------------------------
	.global el3_mdelay

el3_mdelay:

	push	%bp
	mov	%sp,%bp
	mov	4(%bp),%cx	// count
mdly_loop:
	outb	$0x80		// presumably approx 1us
	loop	mdly_loop

	pop	%bp
	ret

