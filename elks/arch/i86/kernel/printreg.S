#include <linuxmt/config.h>

/* This file contains print_regs, which will dump out all of the registers
 * and print them out.  This is probably one of the sickest routines ever
 * written :) - Chad
 */

/*void print_regs(void);*/
/*void printsp(void);*/

#ifndef S_SPLINT_S
#asm
	.extern	_printk
	.globl _print_regs

_print_regs:
	push bp
	push ss
	push es
	push ds
	push cs
	push si
	push di
	push dx
	push cx
	push bx
	push ax
	push #fmtprg
	call _printk
	pop ax
	pop ax
	pop bx
	pop cx
	pop dx
	pop di
	pop si
	pop ds		! Can not pop cs.
	pop ds
	pop es
	pop ss
	pop bp
	ret

	.globl _printsp

_printsp:
	push sp
	push ss
	push #msg
	call _printk
	pop ax
	pop ax
	pop ax
	ret

	.data
msg:	.ascii	"SP=%x:%x\n"
	.byte	0
fmtprg:	.ascii	"AX=%x BX=%x CX=%x DX=%x DI=%x SI=%x\nCS=%x DS=%x ES=%x SS=%x BP=%x\n"
	.byte	0

#endasm
#endif
