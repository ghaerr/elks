#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/wait.h>
#include <linuxmt/sched.h>
#include <linuxmt/config.h>

#include <arch/segment.h>

__arch_mminit arch_segs;
int arch_cpu;			/* Processor type */

void setup_arch(start,end)
int *start;
int *end;
{
	int ct;
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

 	task[0].state = TASK_RUNNING;
	task[0].t_count=0;
	task[0].t_priority=0; /* FIXME why is this here? */
	task[0].t_regs.ksp=task[0].t_kstack+KSTACK_BYTES;
	task[0].t_priority=10;
	task[0].t_count=0;
	task[0].t_regs.cs=get_cs();
	task[0].t_regs.ds=get_ds();	/* Run in kernel space */
	
	current = &task[0];
	
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
		task[ct].state=TASK_UNUSED;
		task[ct].t_kstackm = KSTACK_MAGIC;
	}
	/*
	 *	Fill in the MM numbers - really ought to be in mm not kernel ?
	 */
	
	*end = setupw(0x2a)<<6;
	*start = get_ds();
	*start += ((unsigned int)(_endbss+15))>>4;
}

