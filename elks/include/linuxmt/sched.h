#ifndef LINUXMT_SCHED_H
#define LINUXMT_SCHED_H

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
#include <arch/param.h>
#include <linuxmt/timex.h>
#ifdef CONFIG_STRACE
#include <linuxmt/strace.h>
#endif

struct file_struct
{
	unsigned long close_on_exec;
	struct file *fd[NR_OPEN];
};

struct fs_struct {
	unsigned short umask;
	struct inode * root, * pwd;
};

struct mm_struct {
	unsigned short cseg,dseg;
};

struct signal_struct {
	struct sigaction action[32];
};

struct task_struct 
{
/* Executive stuff */
	__registers t_regs;
	__uint t_num;
	__pptr t_begcode, t_endcode, t_begdata, t_endtext, t_enddata, t_begstack, t_endstack, t_endbrk; 
/* Kernel info */
	pid_t pid,ppid;
	pid_t session;
	uid_t uid,euid,suid;
	gid_t gid,egid,sgid;	
/* Scheduling + status variables */
	__s16 state;
        __u32 timeout;                  /* for select() */
	__uint t_count, t_priority;	/* priority scheduling elements */
/*	__u16 t_flags; 		/* Not defined yet */
	__s32 counter;		/* Time counter (unused so far) */
        struct task_struct *next_run, *prev_run;
        struct task_struct *next_task, *prev_task;
	struct file_struct files;	/* File system structure */
	struct fs_struct fs;		/* File roots */
	struct mm_struct mm;		/* Memory blocks */
	struct signal_struct sig;	/* Signal block */
	gid_t groups[NGROUPS];
/*	int dumpable;		/* Can core dump */
	pid_t pgrp;
	__u8 link_count;	/* Symlink loop counter */
	__u32 signal/*,blocked*/;	/* Signal status */
	/* only 1 child pntr is needed to get us into the sibling list */
	struct task_struct *p_parent, *p_prevsib, *p_nextsib, *p_child;	 
  	struct wait_queue child_wait;
	pid_t child_lastend;
	int lastend_status;
	struct inode * t_inode;
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

/*
 * Scheduling policies
 */
#define SCHED_OTHER             0
#define SCHED_FIFO              1
#define SCHED_RR                2

struct sched_param {
        int sched_priority;
};

/* We use typedefs to avoid using struct foobar (*) */
typedef struct task_struct __task;
typedef struct task_struct * __ptask;

__task task[MAX_TASKS];

extern unsigned long jiffies;
extern __ptask current, next;
extern __uint curnum;
extern int need_resched;

_FUNCTION(__uint build_task, ());
_FUNCTION(__uint alloc_task, (__ptask ptask));
_FUNCTION(void schedule, ()); 

extern struct timeval xtime;
extern unsigned long jiffies;
#define CURRENT_TIME ((xtime.tv_sec) + (jiffies/HZ))

#define for_each_task(p) \
	for (p = &task[0] ; p!=&task[MAX_TASKS]; p++ )

/* void add_wait_queue(struct wait_queue ** p, struct wait_queue * wait); */

#endif
