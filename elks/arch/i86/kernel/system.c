#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/wait.h>
#include <linuxmt/sched.h>
#include <linuxmt/config.h>

#include <arch/segment.h>

__arch_mminit arch_segs;
int arch_cpu;			/* Processor type */

void setup_arch(start,end)
seg_t *start;
seg_t *end;
{
	int ct;
	register __ptask taskp;
/*
 *	Save segments
 */
#asm
	mov bx, #_arch_segs
	mov ax, cs
	mov [bx], ax
	mov [bx+2], di
	mov ax, ss
	mov [bx+8], ax
	mov [bx+6], si
!	This is out of order to save a segment load and a few bytes :)
	mov ax, ds
	mov [bx+4], ax
	mov [bx+10], dx
!	mov ds, ax
#endasm
	arch_segs.lowss = arch_segs.endss;
	/*
	 *	Now create task 0 to be ourself. Set the kernel SP,
	 *	as we will need this in interrupts.
	 */	

	taskp = &task[0];
 	taskp->state = TASK_RUNNING;
#ifdef OLD_SCHED
	taskp->t_count=0;
	taskp->t_priority=0; /* FIXME why is this here? */
#endif /* OLD_SCHED */
	taskp->t_regs.ksp=taskp->t_kstack+KSTACK_BYTES;
#ifdef OLD_SCHED
	taskp->t_priority=10;
	taskp->t_count=0;
#endif /* OLD_SCHED */
	taskp->t_regs.cs=get_cs();
	taskp->t_regs.ds=get_ds();	/* Run in kernel space */
	
	current = taskp;
	
#ifdef CONFIG_COMPAQ_FAST
	/*
	 *	Switch COMPAQ Deskpto to high speed
	 */
	outb_p(1,0xcf);
#endif

	/*
	 *	Mark tasks 1-31 as not in use.
	 */
	for(ct=1;ct<MAX_TASKS;ct++) {
/* 		taskp = &task[ct]; */
		taskp++;
		taskp->state=TASK_UNUSED;
		taskp->t_kstackm = KSTACK_MAGIC;
	}
	/*
	 *	Fill in the MM numbers - really ought to be in mm not kernel ?
	 */
	
	*end = setupw(0x2a)<<6;
	*start = get_ds();
	*start += ((unsigned int)(_endbss+15))>>4;
}

#asm
#ifndef CONFIG_SHLIB
export _sys_dlload
_sys_dlload:
#endif
#ifndef CONFIG_SOCKET
export _sys_socket
_sys_socket:
export _sys_bind
_sys_bind:
export _sys_listen
_sys_listen:
export _sys_accept
_sys_accept:
export _sys_connect
_sys_connect:
#endif
export _no_syscall
_no_syscall:
	mov ax,#-38
	ret
#endasm


