/*
 *	Architecture specific C bootstrap
 */
 
#include <arch/segment.h>
 
#ifdef USE_C

void arch_boot(void)
{
    /*
     *	Wipe the BSS
     */
    short *ptr = _enddata;

    while(ptr < _endbss)
	*ptr++ = 0;
}

#else

#ifndef S_SPLINT_S
#asm

    .globl _arch_boot

    .text

_arch_boot:
    push di
    mov di,__enddata
    mov cx,__endbss
    sub cx,di
    xor ax,ax
    shr cx,#1
    rep
    stosw
    pop di
    ret

#endasm
#endif

#endif
