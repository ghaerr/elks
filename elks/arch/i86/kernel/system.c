#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/wait.h>
#include <linuxmt/sched.h>
#include <linuxmt/config.h>

#include <arch/segment.h>

__arch_mminit arch_segs;
int arch_cpu;			/* Processor type */
extern long int basmem;

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
#ifndef CONFIG_ARCH_SIBO	
	*end = setupw(0x2a)<<6;
	*start = get_ds();
	*start += ((unsigned int)(_endbss+15))>>4;
#else /* CONFIG_ARCH_SIBO */
	*end = (basmem)<<6;
	*start = get_ds();
	*start += (unsigned int)0x1000;
#endif /* CONFIG_ARCH_SIBO */
}

#asm
#ifdef CONFIG_NOFS
export _sys_access
export _sys_chdir
export _sys_chmod
export _sys_chown
export _sys_chroot
export _sys_fchown
export _sys_fstat
export _sys_insmod
export _sys_link
export _sys_lstat
export _sys_mkdir
export _sys_mknod
export _sys_mount
export _sys_readdir
export _sys_readlink
export _sys_rename
export _sys_rmdir
export _sys_stat
export _sys_symlink
export _sys_sync
export _sys_umask
export _sys_umount
export _sys_unlink
export _sys_utime
_sys_access:
_sys_chdir:
_sys_chmod:
_sys_chown:
_sys_chroot:
_sys_fchown:
_sys_fstat:
_sys_insmod:
_sys_link:
_sys_lstat:
_sys_mkdir:
_sys_mknod:
_sys_mount:
_sys_readdir:
_sys_readlink:
_sys_rename:
_sys_rmdir:
_sys_stat:
_sys_symlink:
_sys_sync:
_sys_umask:
_sys_umount:
_sys_unlink:
_sys_utime:
#endif
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


