#include <arch/irq.h>
#include <linuxmt/config.h>

/*
 *	Easy way to store our kernel DS
 */


/* moving variables from code segment to an extra segment
/  CONFIG_ROM_IRQ_DATA for the ROM_CODE-Version
/  ELKS 0.76 7/1999 Christian Mard”ller  (chm@kdt.de)
/  */



#ifdef CONFIG_ROMCODE 
   #define stashed_ds       [0]
   #define off_stashed_irq0 [2]
   #define seg_stashed_irq0 [4]
   #define stashed_ss       [6]
   #define stashed_sp       [8]
   #define stashed_irq      [10]  
   #define bios_call_cnt    [12] 
   /*      stashed_di       [14]   process.c */
   /*      cs_tmp           [16]   process.c */
   /*      _our_ds          [18]   bios16.c  */

#else
   #define stashed_ds       cseg_stashed_ds
   #define seg_stashed_irq0 cseg_seg_stashed_irq0
   #define off_stashed_irq0 cseg_off_stashed_irq0

   #define stashed_ss       cseg_stashed_ss
   #define stashed_sp       cseg_stashed_sp
   #define stashed_irq      cseg_stashed_irq
   #define bios_call_cnt    cseg_bios_call_cnt
#endif



#asm
/* In ROM-Mode we must generate a physical 3th segment :-) 
/  The segmentaddress is given by CONFIG_ROM_IRQ_DATA,
/  the offset is constant per #define
/-------------------------------------------------------*/



#ifndef CONFIG_ROMCODE
	.globl cseg_stashed_ds
	.globl cseg_stashed_si
	.globl cseg_sc_tmp
	.globl IRQdata_offs

        .even

cseg_stashed_ds:
	.word	0
cseg_seg_stashed_irq0:
	.word	0
cseg_off_stashed_irq0:
	.word	0


cseg_stashed_ss:
	.word 	0
cseg_stashed_sp:
	.word 	0
cseg_stashed_irq:
	.word	0
cseg_bios_call_cnt:
	.word	0


;from process.c

cseg_stashed_si:
	.word 	0
cseg_sc_tmp:
	.word	0
IRQdata_offs:
        .word   0


#endif	
	

#endasm





 
void irqtab_init()
{
#asm

; CS points to this kernel code segment
; ES points to page 0  (interrupt table)
; DS points to the irqdataseg (cs or CONFIG_ROM_IRQ_DATA)

        push ds
        mov dx,ds      ;the original value
        cli            ;just here
        
        xor ax,ax
        mov es,ax      ;intr table

#ifdef CONFIG_ROMCODE
        mov ax,#CONFIG_ROM_IRQ_DATA
#else
        mov ax,cs
#endif        
        mov ds,ax
        
	mov stashed_ds,dx	

	seg es                     ;insert new timer intr 
	mov bx,[32]
	mov off_stashed_irq0, bx   ; the old one
	lea ax,_irq0
	seg es
	mov [32],ax
	seg es
	mov bx,[34]
	mov seg_stashed_irq0, bx
	mov ax,cs
	seg es
	mov [34],ax


#ifndef CONFIG_CONSOLE_BIOS
	lea ax,_irq1      ;keyboard  
	seg es
	mov [36],ax
	mov ax,cs
	seg es
	mov [38],ax
#endif

#if 0	
	lea ax,_irq2
	seg es
	mov [40],ax
	mov ax,cs
	seg es
	mov [42],ax
#endif	

	lea ax,_irq3      ;com2
	seg es
	mov [44],ax
	mov ax,cs
	seg es
	mov [46],ax
	
	lea ax,_irq4     ;com1
	seg es
	mov [48],ax
	mov ax,cs
	seg es
	mov [50],ax
	

! Setup INT 0x80 (for syscall)
	lea ax,_syscall_int
	seg es
	mov [512],ax
	mov ax,cs
	seg es
	mov [514],ax
! Tidy up

	pop ds           ;the org segments
	mov ax,ds
	mov es,ax
	sti
        
#endasm
}	
 
/*
 *	IRQ and IRQ return paths for Linux 8086
 */
 
#asm
!
!	Other IRQs (see IRQ 0 at the bottom for the
!	main code).
!
	.text
	.globl _irq1
!	.globl _irq2
	.globl _irq3
	.globl _irq4
!	.globl _irq5
!	.globl _irq6
!	.globl _irq7
!	.globl _irq8
!	.globl _irq9
!	.globl _irq10
!	.globl _irq11
!	.globl _irq12
!	.globl _irq13
!	.globl _irq14
!	.globl _irq15
	.extern _do_IRQ

	.data
	.extern _cache_A1
	.extern _cache_21
	.extern _jiffies

	.text 
			

_irq1:                   ;keyboard  
	push	ax
	mov	ax,#1
	br	_irqit
#if 0
_irq2:
	push	ax
	mov	ax,#2
	br	_irqit
#endif
_irq3:                   ;com2
	push	ax
	mov	ax,#3
	br	_irqit
_irq4:                   ;com1
	push	ax
	mov	ax,#4
	br	_irqit

#if 0
_irq5:
	push	ax
	mov	ax,#5
	br	_irqit
_irq6:
	push	ax
	mov	ax,#6
	br	_irqit
_irq7:
	push	ax
	mov	ax,#7
	br	_irqit
!
!	AT interrupts
!	
_irq8:
	push	ax
	mov	ax,#8
	br	_irqit
_irq9:
	push	ax
	mov	ax,#9
	br	_irqit
_irq10:
	push	ax
	mov	ax,#10
	br	_irqit
_irq11:
	push	ax
	mov	ax,#11
	jmp	_irqit
_irq12:
	push	ax
	mov	ax,#12
	jmp	_irqit
_irq13:
	push	ax
	mov	ax,#13
	jmp	_irqit
_irq14:
	push	ax
	mov	ax,#14
	jmp	_irqit
_irq15:
	push	ax
	mov	ax,#15
	jmp	_irqit
#endif
!
!
!	Traps (we use IRQ 16->31 for these)
!
!	Currently not used so removed for space.
#if 0
	.globl _div0
_div0:
	push	ax
	mov	ax,#16
	jmp	_irqit

	.globl _dbugtrap
_dbugtrap:
	push	ax
	mov	ax,#17
	jmp	_irqit

	.globl _nmi
_nmi:
	push	ax
	mov	ax,#18
	jmp	_irqit

	.globl	_brkpt
_brkpt:
	push 	ax
	mov	ax,#19
	jmp	_irqit

	.globl	_oflow
_oflow:
	push	ax
	mov	ax,#20
	jmp	_irqit

	.globl	_bounds
_bounds:
	push	ax
	mov	ax,#21
	jmp	_irqit
	
	.globl	_invop
_invop:
	push	ax
	mov	ax,#22
	jmp	_irqit
	
	.globl _devnp
_devnp:
	push	ax
	mov	ax,#23
	jmp	_irqit

	.globl	_dfault
_dfault:
	push	ax
	mov	ax,#24
	jmp	_irqit
;
;	trap 9 is reserved
;	
	.globl _itss
_itss:
	push	ax
	mov	ax,#26
	jmp	_irqit

	.globl _nseg
_nseg:
	push	ax
	mov	ax,#27
	jmp	_irqit
	
	.globl _stkfault
_stkfault:
	push 	ax
	mov	ax,#28
	jmp	_irqit

	.globl	_segovr
_segovr:
	push	ax
	mov	ax,#29
	jmp	_irqit
	
	.globl _pfault
_pfault:
	push	ax
	mov	ax,#30
	jmp	_irqit
;
;	trap 15 is reserved
;
	.globl	_fpetrap
_fpetrap:
	push	ax
	mov	ax,#32
	jmp	_irqit

	.globl	_algn
_algn:
	push	ax
	mov	ax,#33
	jmp	_irqit

#endif
!
!	On entry CS:IP is all we can trust
!
!	There are three possible cases to cope with
!
!	SS = kernel DS. 
!		Interrupted kernel mode code.
!		No task switch allowed
!		Running on a kernel process stack anyway.
!
!	SS = current->user_ss
!		Interrupted user mode code
!		Switch to kernel stack for process (will be free)
!		Task switch allowed
!
!	Other
!		BIOS or other 'strange' code.
!		Must be called from kernel space, but kernel stack is in use
!		Switch to int_stack
!		No task switch allowed.
!
!	We do all of this to avoid per process interrupt stacks and 
!	related nonsense. This way we need only one dedicted int stack
!
!
	.globl	_irq0
_irq0:
!
!	Save AX and load it with the IRQ number
!
	push	ax
	xor	ax,ax
_irqit:
!
!	Save all registers
!
	cli		! Might not be disabled on an exception
	push	ds
	push	es
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	bp
!
!	Recover segments
!
#ifdef CONFIG_ROMCODE
        mov bx,#CONFIG_ROM_IRQ_DATA
#else
        mov bx,cs
#endif        
        mov ds,bx
        
	mov	stashed_irq,ax	! Save IRQ number
	mov	ax,ss		! Get current SS
	mov	bx,ax		! Save for later
	mov	stashed_ss, ax	! Save SS:SP
	mov	ax,sp
	mov	stashed_sp, ax
!
!	Switch segments
!
	mov	ax,stashed_ds   ! Recover the data segment
	mov	ds,ax
	mov	es,ax
!
!	Set up task switch controller
!
	xor	ch,ch		! Assume we are not allowed to switch
!
!	See where we were (BX holds the SS on entry)
!
	cmp	ax,bx		! SS = kernel SS ?
	je	ktask		! Kernel - no work
!
!	User or BIOS etc
!
	mov	ax,bx
	mov	bx,_current
	cmp	ax,4[bx]	! entry ss = current->t_regs.ss?
	je	utask		! Switch to kernel
!
!	Bios etc - switch to interrupt stack
!
	mov	sp,#_intstack
!	lea	sp, _intstack
	j	switched
!
!	User task. Extract kernel SP. (BX already holds current)
!
utask:
	mov	ax,[bx]		! kernel stack ptr
	mov	sp,ax		! switch to kernel stack
	inc	ch		! Switch allowable
	j	switched
ktask:
!
!	In ktask state we have a suitable stack. It might be 
!	better to use the intstack..
!
switched:
	mov	ax,ds
	mov	ss,ax		! /* Set SS: right */
! /*
!	Put the old SS;SP on the top of the stack. We can't
!	leave them in stashed_ss/sp as we could re-enter the
!	routine on a reschedule.
! */
#ifdef CONFIG_ROMCODE
        mov ax,#CONFIG_ROM_IRQ_DATA
        mov es,ax
        seg es
	push	stashed_sp
	seg es
	push	stashed_ss
        
#else
	seg 	cs
	push	stashed_sp
	seg	cs
	push	stashed_ss
#endif
!
!	We are on a suitable stack and cx says whether we can	
!	switch afterwards. The C code will want to eat CX so
!	we have to hide it
!
!
!	The registers are now stored. Remember where
!
	mov	bp,sp
	mov	_can_tswitch, ch
	push	cx		! Save ch
#ifdef CONFIG_ROMCODE
        seg	es
#else        
	seg	cs		! Recover the IRQ we saved
#endif	
	mov	ax,stashed_irq
	push	ax		! IRQ for later
	push	bp		! Register base
	push	ax		! IRQ number
#ifdef CONFIG_ROMCODE
        mov ax,ds
        mov es,ax        ;es back to dataseg
#endif        
!
!	Call the C code
!
	call	_do_IRQ		! Do the work
!
!	Return path
!
	pop	ax		! We want the ax value back
	pop	bx		! Drop arguments
	pop	ax		! Saved IRQ
	pop	cx		! Recover switch allowed flag
!
!	Restore any chips
!
	cmp	ax,#15
	jge	was_trap	! Traps need no reset
	cmp	ax,#8
	jge	sec_8259	! IRQ on low chip
!
!	Reset primary 8259
!
	mov	cl,al		! Save the IRQ number
	inb	al,0x21		! The chip line state
	jmp	a7
a7:	jmp	a8
a8:
!	movb	al,#1
!	shl	al,cl		! Shift the irq (saved in cl) to a mask
!	orb	al,_cache_21
!	movb	_cache_21, al	
	movb	al,_cache_21	! Extract the IRQ mask register
	outb	0x21,al		! Now ack the IRQ
	jmp	a9
a9:	jmp	a10
a10:	movb	al,#0x20	! EOI
	outb	0x20,al
	jmp	was_trap
	
!
!	Reset secondary 8259 if we have taken an AT rather
!	than XT irq. We also have to prod the primay
!	controller EOI..
!
sec_8259:
	mov	cl,al		! Save the IRQ for making masks
	inb	al,0xA1
	jmp	a1
a1:	jmp	a2
a2:	movb	al,#1
	shl	al,cl
	orb	al,_cache_A1
	movb	_cache_A1, al
	outb	0xA1,al		! Now ack the IRQ
	jmp	a3
a3:	jmp	a4
a4:	movb	al,#0x20
	outb	0xA0,al
	jmp	a5
a5:	jmp	a6
a6:	outb	0x20,al		! Ack on primary controller

!
!	And a trap does no hardware work	
!

was_trap:
!
!	Now look at rescheduling
!
	cmp	ch,#0			! Schedule allowed ?
	je	nosched			! No
!	mov	bx,_need_resched	! Schedule needed
!	cmp	bx,#0			! 
!	je	nosched			! No
	call	_schedule		! Task switch
!
! Fix current->ksp (_schedule messes it up).
!
	pop	ax	! stacked SS
	pop	cx	! stacked SP
	mov	bx,_current
	mov	[bx],sp
	j	noschedpop
	
nosched:
!
!	Now we have to rescue our stack pointer/segment.
!
	pop	ax	! SS
	pop	cx	! SP
!
!	Switch stacks to the interrupting stack
!
noschedpop:
	mov	ss,ax
	mov	sp,cx
!
!	Restore registers and return
!
	pop	bp
	pop 	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	es
#ifdef CONFIG_ROMCODE
	mov	ax,#CONFIG_ROM_IRQ_DATA
	mov	ds,ax
#else
	seg	cs
#endif
	mov	ax, stashed_irq
	or 	ax,ax
	jz	irq0_bios
	pop	ds
	pop	ax
!
!	Iret restores CS:IP and F (thus including the interrupt bit)
!
	iret
!
!	IRQ 0 (timer) has to go on to the bios for some systems
!	
!	FIXME: should call the bios only every fifth event.
!
irq0_bios:
        pop     ds  
	pop	ax           ;now the stack empty

;------------------------------------------------
;Build new Stack
;
;  SP    ->  RET seg
;            RET offs
;  SP-4  ->  BP
;  SP-4  ->  BX   
;            DS
;  SP-8  ->  free                     ;sp

label1:

        sub sp,#4                     ;space for retf
        push bp
        mov bp,sp 

	push	bx
	push    ds                 
#ifdef CONFIG_ROMCODE
        mov bx,#CONFIG_ROM_IRQ_DATA
#else
        mov bx,cs 
#endif
        mov ds,bx
	mov	bx,bios_call_cnt
	inc	bx
	cmp	bx,#5
	jne	no_bios_call

	xor	bx,bx
	mov	bios_call_cnt,bx
        mov bx, seg_stashed_irq0
	mov	[bp+4], bx
        mov bx, off_stashed_irq0
        mov 	[bp+2], bx

        pop     ds
	pop	bx       
        pop bp
	retf                          

no_bios_call:                          ;sp-8 
	mov	bios_call_cnt,bx
        pop	ds
	pop	bx                     ;sp-4              
        pop     bp
        add sp,#4
	iret


	.data
.globl	_can_tswitch
_can_tswitch:
	.byte 0
	.zerow	256		! (was) 128 byte interrupt stack
_intstack:

#endasm
