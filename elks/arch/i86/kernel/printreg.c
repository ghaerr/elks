#include <linuxmt/config.h>

/* This file contains print_regs, which will dump out all of the registers
 * and print them out.  This is probably one of the sickest routines ever
 * written :) - Chad */

void print_regs2(ax, bx, cx, dx, di, si, cs, ds, es, ss, bp)
int ax, bx, cx, dx, di, si, cs, ds, es, ss, bp;
{
	printk("AX=%x BX=%x CX=%x DX=%x DI=%x SI=%x\nCS=%x DS=%x ES=%x SS=%x BP=%x\n", ax, bx, cx, dx, di, si, cs, ds, es, ss, bp);
}

#asm
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
	call _print_regs2
	pop ax
	pop bx
	pop cx
	pop dx
	pop di
	pop si
	pop ds	! /*  Can't pop cs */
	pop ds
	pop es
	pop ss
	pop bp
	ret
#endasm
