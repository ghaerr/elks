#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>

void memdumpk(int seg,int off,int len)
{
    while (len > 0) {
	int i;
	printk("%x:%x", seg, off);
	for (i=0; i<8; i++)
	    printk(" %x", peekb(seg, off++));
	printk("\n");
	len -= 8;
    }
}

void printsp(void)
{
#ifndef S_SPLINT_S
#asm

    .data
msg:	.ascii	"SP=%x:%x\n"
	.byte	0

    .text
	push sp
	push ss
	push #msg
	call _printk
	pop ax
	pop ax
	pop ax
	ret

#endasm
#endif
}
