#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/types.h>

/* This file contains print_regs, which will dump out all of the registers
 * and print them out.  This is probably one of the sickest routines ever
 * written :) - Chad
 */

void print_regs2(__u16 ax,__u16 bx,__u16 cx,__u16 dx,__u16 di,__u16 si,
		 __u16 cs,__u16 ds,__u16 es,__u16 ss,__u16 bp)
{
    printk("AX=%x BX=%x CX=%x DX=%x DI=%x SI=%x\n", ax, bx, cx, dx, di, si);
    printk("CS=%x DS=%x ES=%x SS=%x BP=%x\n", cs, ds, es, ss, bp);
}

#ifndef S_SPLINT_S
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
	pop ds		! Can not pop cs.
	pop ds
	pop es
	pop ss
	pop bp
	ret
#endasm
#endif
