#include <arch/irq.h>
#include <arch/asm-offsets.h>
#include <linuxmt/config.h>

/*
 *	Easy way to store our kernel DS
 */

/* moving variables from code segment to an extra segment
/  CONFIG_ROM_IRQ_DATA for the ROM_CODE-Version
/  ELKS 0.76 7/1999 Christian Mard”ller  (chm@kdt.de)
/  */

#ifdef CONFIG_ROMCODE 
/* In ROM-Mode we must generate a physical 3th segment :-)
/  The segmentaddress is given by CONFIG_ROM_IRQ_DATA,
/  the offset is constant per #define
/-------------------------------------------------------*/

   #define SEG_IRQ_DATA es
   #define stashed_ds       [0]

#else
 #define SEG_IRQ_DATA cs

#ifndef S_SPLINT_S
#asm
        .globl stashed_ds

        .even

stashed_ds:
	.word	0

#endasm
#endif

#endif

extern void sig_check(void);

void irqtab_init(void)
{
#ifndef S_SPLINT_S
#asm

; CS points to this kernel code segment
; ES points to page 0  (interrupt table)
; DS points to the kernel data segment

        cli
        
#ifdef CONFIG_ROMCODE
        mov ax,#CONFIG_ROM_IRQ_DATA
        mov es,ax
#endif        
        
        seg SEG_IRQ_DATA
	mov stashed_ds,ds
	mov bios_call_cnt_l,#5
        mov _intr_count,#0

        xor ax,ax
        mov es,ax      ;intr table

	seg es
	mov ax,[32]
	mov off_stashed_irq0_l, ax   ; the old timer intr
	seg es
	mov ax,[34]
	mov seg_stashed_irq0_l, ax

	seg es
	mov [32],#_irq0   ;timer
	seg es
	mov [34],cs

#ifndef CONFIG_CONSOLE_BIOS
	seg es
        mov [36],#_irq1   ;keyboard
	seg es
        mov [38],cs
#endif

#if 0	
	lea ax,_irq2
	seg es
	mov [40],ax
	mov ax,cs
	seg es
	mov [42],ax
#endif	

	seg es
        mov [44],#_irq3   ;com2
	seg es
        mov [46],cs
	
	seg es
	mov [48],#_irq4   ;com1
	seg es
	mov [50],cs
	

! Setup INT 0x80 (for syscall)
	seg es
	mov [512],#_syscall_int
	seg es
	mov [514],cs
! Tidy up

        mov dx,ds      ;the original value
        mov es,dx      ;just here
	sti
        
#endasm
#endif
}	
 
/*
 *	IRQ and IRQ return paths for Linux 8086
 */

#ifndef S_SPLINT_S
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
        mov ds,bx
#else
        seg     cs
#endif        
	mov	bx,stashed_ds		! Recover the data segment
	mov	ds,bx
	mov	es,bx

	mov	dx,ss			! Get current SS
	mov	bp,sp			! Get current SP
!
!	Set up task switch controller
!
	xor	ch,ch		! Assume we are not allowed to switch
!
!       See where we were (DX holds the SS on entry)
!
	cmp	dx,bx		! SS = kernel SS ?
	je	ktask		! Kernel - no work
!
!	User or BIOS etc
!
        mov     ss,bx           ! /* Set SS: right */
	mov	bx,_current
	cmp	dx,TASK_USER_SS[bx] ! entry ss = current->t_regs.ss?
	jne	btask		! Switch to interrupt stack
!
!	User task. Extract kernel SP. (BX already holds current)
!	At this point, the kernel stack is empty. Thus, we can load
!       the kernel stack pointer without accesing memory
!
        mov     TASK_USER_SP[bx],sp
        lea     sp,TASK_KSTKTOP[bx] ! switch to kernel stack ptr
        lea     bp,TASK_USER_SP[bx]
	inc	ch		! Switch allowable
        j       updct
!
!	Bios etc - switch to interrupt stack
!
btask:
	mov	sp,#_intstack
!
!	In ktask state we have a suitable stack. It might be 
!	better to use the intstack..
!
ktask:
! /*
!	Put the old SS;SP on the top of the stack. We can't
!	leave them in stashed_ss/sp as we could re-enter the
!	routine on a reschedule.
! */
	push	dx		! push entry SS
	push	bp		! push entry SP
!
!	The registers are now stored. Remember where
!
	mov	bp,sp
!
!   Update intr_count
!
updct:
        inc     _intr_count
!
!       We are on a suitable stack and ch says whether
!       we can switch afterwards.
!
        sti                     ! Reenable interrupts
        push    cx              ! Switch flag
	push	ax		! IRQ for later
	push	bp		! Register base
	push	ax		! IRQ number
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
        cli                     ! Disable interrupts to avoid reentering ISR
	cmp	ax,#16
	jge	was_trap	! Traps need no reset
	or	ax,ax		! Is int #0?
	jnz	a4
!
!        IRQ 0 (timer) has to go on to the bios for some systems
!
        dec     bios_call_cnt_l	! Will call bios int?
        jne     a4
        mov     bios_call_cnt_l,#5
        pushf
        callf   [off_stashed_irq0_l]
        jmp     was_trap
a4:
        cmp     ax,#8
	movb	al,#0x20	! EOI
	jb	a6		! IRQ on low chip
!
!	Reset secondary 8259 if we have taken an AT rather
!	than XT irq. We also have to prod the primay
!	controller EOI..
!
	outb	0xA0,al
	jmp	a5
a5:	jmp	a6
a6:	outb	0x20,al		! Ack on primary controller
!
!	And a trap does no hardware work	
!
was_trap:
!
!   Restore intr_count
!
        dec     _intr_count
!
!	Now look at rescheduling
!
        orb     ch,ch                   ! Schedule allowed ?
	je	nosched			! No
!	mov	bx,_need_resched	! Schedule needed
!	cmp	bx,#0			! 
!	je	nosched			! No
!
! This path will return directly to user space
!
	call	_schedule		! Task switch
        mov     bx,_current
        mov     8[bx],#1
        call    _sig_check              ! Check signals
!
!	At this point, the kernel stack is empty. Thus, there is no
!       need to save the kernel stack pointer.
!
        mov     bx,_current
        mov     sp,TASK_USER_SP[bx]
#ifndef CONFIG_ADVANCED_MM
        mov     ss,TASK_USER_SS[bx]
#else
	mov ax, TASK_USER_SS[bx] ! user ds
	mov bp, sp
        mov ss, ax
	mov 12[bp], ax	! change the es in the stack
	mov 14[bp], ax	! change the ds in the stack
#endif
	j	noschedpop
!
!	Now we have to rescue our stack pointer/segment.
!
nosched:
	pop	cx	! SP
	pop	ss	! SS
	mov	sp,cx
!
!	Restore registers and return
!
noschedpop:
	pop	bp
	pop 	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	es
	pop	ds
	pop	ax
!
!	Iret restores CS:IP and F (thus including the interrupt bit)
!
	iret

	.data
        .globl  _intr_count
_intr_count:
        .word 0

off_stashed_irq0_l:
	.word	0
seg_stashed_irq0_l:
	.word	0
bios_call_cnt_l:
	.word	0

	.zerow	256		! (was) 128 byte interrupt stack
_intstack:

#endasm
#endif
