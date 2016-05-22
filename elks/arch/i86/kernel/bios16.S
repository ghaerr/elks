/*
 *	16bit PC BIOS interface library
 *
 *	FIXME: Only supports IRQ 0x13 for now. Needs to do 0x10 later!
 *
 *   9/1999  place the CS-Variable in a seperated RAM-Segment for
 *           ROM version. Christian Mardm"oller (chm@kdt.de)
 */

#include <linuxmt/config.h>

/*
 *	Quick drop into assembler for this one.
 */

#ifndef S_SPLINT_S
#asm
	.text
/*
 *	stashed_ds lives in the kernel cs or we can never recover it...
 *
 * In ROM we cant store anything! The space for the extrasegment
 * is managed in irqtab.c where I've found this mode first
 * ChM 10/99
 */

#ifdef CONFIG_ROMCODE
#define stashed_ds	[0]
#else 
	.extern stashed_ds
#endif	
	
	.globl  _call_bios

_call_bios:
	push bp
	mov  bp,sp

! Things we want to save - direction flag BP ES

	pushf
	push es
	push si
	push di

! DS already saved in stashed_ds

	mov bx,4[bp]

!	Load the register block from the table	

	mov ax,2[bx]
	mov cx,6[bx]
	mov dx,8[bx]
	mov si,10[bx]
	mov di,12[bx]
	mov bp,14[bx]
	mov es,16[bx]
	push 18[bx]        ! DS in stack
	push 20[bx]
	popf
	mov bx, 4[bx]      ! Load BX
!
!	Stack now holds the call value for DS
!
	pop ds             ! DS desired

! ***** DS is now wrong we cannot load from the array again *****

!	Do a disk interrupt.

	int #0x13

!	Now recover the results

!	Make some breathing room

	pushf
	push ds
	push bx

!	Stack is now returned FL, BX, DS

!	Recover our DS segment

#ifdef CONFIG_ROMCODE
        mov bx,#CONFIG_ROM_IRQ_DATA
        mov ds,bx       ;we can use ds for one fetch
#else
	seg cs		
#endif
        mov ds,stashed_ds  ! the org DS of kernel

! ***** We can now use the bios data table again *****

	mov bx,sp
	seg ss
	mov bx,18[bx]

	mov 2[bx],ax       ! Save the old AX
	pop 4[bx]          ! Save the old BX
	mov 6[bx], cx
	mov 8[bx], dx
	mov 10[bx], si
	mov 12[bx], di
	mov 14[bx], bp
	mov 16[bx], es
	pop 18[bx]         ! Save the old DS
	pop 20[bx]         ! Pop the returned flags off

!	Restore things we must save

	pop di
	pop si
	pop es
	popf
	pop bp
	ret

#endasm
#endif
