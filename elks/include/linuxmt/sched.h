#ifndef __LINUXMT_SCHED_H__
#define __LINUXMT_SCHED_H__

#define MAX_TASKS 15
#define NGROUPS	13		/* Supplementary groups */
#define NOGROUP 0xFFFF
#define KSTACK_BYTES 1024	/* Size of kernel stacks */
#define USTACK_BYTES 4096	/* Size of user-mode stacks */

#include <linuxmt/fs.h>
#include <linuxmt/time.h>
#include <linuxmt/signal.h>
#include <linuxmt/autoconf.h>
#include <linuxmt/wait.h>
#include <linuxmt/ntty.h>
#include <arch/param.h>
#include <linuxmt/timex.h>
#ifdef CONFIG_STRACE
#include <linuxmt/strace.h>
#endif

struct file_struct
{
	fd_mask_t close_on_exec;
	struct file *fd[NR_OPEN];
};

struct fs_struct {
	unsigned short umask;
	struct inode * root, * pwd;
};

struct mm_struct {
	seg_t cseg,dseg;
	char flags;
#define CS_SWAP		1
#define DS_SWAP		2
};

struct signal_struct {
	struct sigaction action[NSIG];
};

struct task_struct 
{
/* Executive stuff */
	__registers t_regs;
	__pptr t_enddata, t_begstack, t_endbrk, t_endseg;
/* Kernel info */
	pid_t pid,ppid;
	pid_t session;
	uid_t uid,euid,suid;
	gid_t gid,egid,sgid;	
/* Scheduling + status variables */
	__s16 state;
        __u32 timeout;                  /* for select() */
	struct wait_queue *waitpt;	/* Wait pointer */
	__u16 pollhash;
	struct task_struct *next_run, *prev_run;
	struct file_struct files;	/* File system structure */
	struct fs_struct fs;		/* File roots */
	struct mm_struct mm;		/* Memory blocks */
	pid_t pgrp;
	struct tty * tty;
/*	__u8 link_count;		/* Symlink loop counter, now global */
	struct task_struct *p_parent, *p_prevsib, *p_nextsib, *p_child;	 
	struct wait_queue child_wait;
	pid_t child_lastend;
	int lastend_status;
	struct inode * t_inode;
	sigset_t signal;		/* Signal status */
	struct signal_struct sig;	/* Signal block */
	int dumpable;			/* Can core dump */
#ifdef CONFIG_OLD_SCHED
	__uint t_count, t_priority;	/* priority scheduling elements */
	__s32 counter;			/* Time counter (unused so far) */
        struct task_struct *next_task, *prev_task;
#endif /* CONFIG_OLD_SCHED */
#ifdef CONFIG_SUPPLEMENTARY_GROUPS
	gid_t groups[NGROUPS];
#endif
#ifdef CONFIG_STRACE
	struct syscall_params sc_info;
#endif
	__u16 t_kstackm;	/* To detect stack corruption */
	__u8 t_kstack[KSTACK_BYTES];
};

#define KSTACK_MAGIC 0x5476

#define TASK_RUNNING 		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE 	2
#define TASK_ZOMBIE		3
#define TASK_STOPPED		4
#define TASK_SWAPPING		5
#define TASK_UNUSED		6
#define TASK_WAITING		7
#define TASK_EXITING		8

/* We use typedefs to avoid using struct foobar (*) */
typedef struct task_struct __task;
typedef struct task_struct * __ptask;

extern __task task[MAX_TASKS];

extern jiff_t jiffies;
extern __ptask current; /* next; */
extern int need_resched;

extern struct timeval xtime;
#define CURRENT_TIME ((xtime.tv_sec) + (jiffies/HZ))

#define for_each_task(p) \
	for (p = &task[0] ; p!=&task[MAX_TASKS]; p++ )

#endif
