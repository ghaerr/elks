/*
 *	16bit PC BIOS interface library
 *
 *	FIXME: Only supports IRQ 0x13 for now. Needs to do 0x10 later!
 *
 *   9/1999  place the CS-Variable in a seperated RAM-Segment for
 *           ROM version. Christian Mardm"oller (chm@kdt.de)
 */
 
#include <linuxmt/types.h>
#include <arch/segment.h>
#include <linuxmt/biosparm.h>
#include <linuxmt/config.h>

static struct biosparms bdt;

/*
 *	The external interface is a pointer..
 */
 
struct biosparms *bios_data_table=&bdt;

/*
 *	Quick drop into assembler for this one.
 */
 
#asm
	.text
/*
 *	our_ds lives in the kernel cs or we can never recover it...
 */

/* In ROM we cant store anything! The space for the extrasegment
 * is managed in irqtab.c where I've found this mode first
 * ChM 10/99
 */
#ifndef CONFIG_ROMCODE
cseg_our_ds: 
	.word	0
   #define our_ds    cseg_our_ds
#else 
   #define our_ds    [18]
#endif	
	
	.globl  _call_bios
_call_bios:
	pushf			
* Things we want to save - direction flag BP ES
	push bp		
	push es	
	push si
	push di
* We have to save DS carefully.	
	mov ax, ds		

#ifdef CONFIG_ROMCODE
        mov bx,#ROM_KERNEL_IRQDATA
        mov es,bx       ;es is already stored
        seg es
#else
   * We can find our DS from CS now.
	seg cs		
#endif
	mov our_ds, ax
	
	mov bx, _bios_data_table
* Load the register block from the table	
	mov cx,6[bx]
	mov dx,8[bx]
	mov si,10[bx]
	mov di,12[bx]
	mov bp,14[bx]
* ES in 16
	mov ax,16[bx]
	mov es,ax
* Flags in 20
	mov ax,20[bx]	
* Flags to end up with	
	push ax		
* AX final 	
	mov ax, 2[bx]	
* Stack is now Flags, AX
	push ax		
* DS value final
	mov ax, 18[bx]  
* Load BX	
	mov bx, 4[bx]	
* Stack now holds stuff to restore followed by the call values 
* for flags,AX 
*********** DS is now wrong we cannot load from the array again **********/
* DS desired
	mov ds,ax
* AX desired	
	pop ax
* Flags desired
	popf
*
*	Do a disk interrupt.
*
	int #0x13
*
*	Now recover the results
*
* Make some breathing room
 	pushf
 	push bx
 	push ax
 	mov  ax,ds
 * Stack is now returned FL, BX, AX, DS
 	push ax 

#ifdef CONFIG_ROMCODE
        mov ax,#ROM_KERNEL_IRQDATA
        mov ds,ax       ;we can use ds for one fetch
#else
   * We can find our DS from CS now.
	seg cs		
#endif
 	mov  ax, our_ds

* Recover our DS segment 	
 	mov  ds, ax
 	mov  bx, _bios_data_table
********** We can now use the bios data table again ***************
  	pop ax
* Save the old DS 
  	mov 18[bx], ax
 	pop ax
* Save the old AX 	
 	mov 2[bx], ax
	pop ax
* Save the old BX	
	mov 4[bx], ax
	mov 6[bx], cx
	mov 8[bx], dx
	mov 10[bx], si
	mov 12[bx], di
	mov 14[bx], bp
	mov ax,es
	mov 16[bx], ax
	pop ax
* Pop the returned flags off
	mov 20[bx], ax
*
*	Restore things we must save
*	 
	pop di
	pop si
	pop es
	pop bp
	popf
	ret
#endasm
	
