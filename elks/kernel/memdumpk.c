#include <linuxmt/kernel.h>

void memdumpk(seg, off, len)
int seg, off, len;
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

void printsp()
{
#asm
	.data
	msg: .ascii "SP=%x:%x\n"
	     .byte 0
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
}

