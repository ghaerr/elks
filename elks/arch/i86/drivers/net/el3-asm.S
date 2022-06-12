//
// Low level routines for ELKS EtherLink III ethernet driver (3c509)
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
// void el3_insw(int port, char *data, int count)
//

	.global el3_insw

el3_insw:

	push    %di
	mov	%sp,%di
	push	%es

	mov	4(%di),%dx	// Port
	mov	8(%di),%cx	// length (words)
	mov	6(%di),%di	// Buffer pointer

        mov     current,%bx
        mov     TASK_USER_DS(%bx),%es	// destination segment = user process
	cli

word_loop:
        in      %dx,%ax
        stosw
        loop    word_loop

	sti
        pop     %es
        pop     %di

        ret


//-----------------------------------------------------------------------------
// Write data to port
//-----------------------------------------------------------------------------
// void el3_sendpk(int port, char *data, int count)
//

	.global el3_sendpk

el3_sendpk:
        push    %si
	mov	%sp,%si
	push	%ds

	mov	4(%si),%dx	// Port
        mov     8(%si),%cx      // byte count
        mov     6(%si),%si      // buffer addr

	cli
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

	sti
	pop     %ds
	pop	%si

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

