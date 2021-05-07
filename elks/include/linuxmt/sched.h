#ifndef __LINUXMT_SCHED_H
#define __LINUXMT_SCHED_H

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/limits.h>
#include <linuxmt/fs.h>
#include <linuxmt/time.h>
#include <linuxmt/signal.h>
#include <linuxmt/wait.h>
#include <linuxmt/ntty.h>
#include <arch/param.h>

#ifdef CONFIG_STRACE
#include <linuxmt/strace.h>
#endif

struct file_struct {
    fd_mask_t			close_on_exec;
    struct file 		*fd[NR_OPEN];
};

struct fs_struct {
    unsigned short int		umask;
    struct inode		*root;
    struct inode		*pwd;
};

struct mm_struct {
    struct segment		*seg_code;
    struct segment		*seg_data;
    char			flags;
};

struct signal_struct {
    __kern_sighandler_t		handler;
    struct __kern_sigaction_struct action[NSIG];
};

struct task_struct {

/* Executive stuff */
    struct xregs		t_xregs;
    __pptr			t_enddata;
    __pptr			t_begstack;
    __pptr			t_endbrk;
    __pptr			t_endseg;
    int				t_minstack;

/* Kernel info */
    pid_t			pid;
    pid_t			ppid;
    pid_t			session;
    uid_t			uid;
    uid_t			euid;
    uid_t			suid;
    gid_t			gid;
    gid_t			egid;
    gid_t			sgid;

/* Scheduling + status variables */
    __s16			state;
    __u32			timeout;	/* for select() */
    struct wait_queue		*waitpt;	/* Wait pointer */
    struct wait_queue		*poll[POLL_MAX];  /* polled queues */
    struct task_struct		*next_run;
    struct task_struct		*prev_run;
    struct file_struct		files;		/* File system structure */
    struct fs_struct		fs;		/* File roots */
    struct mm_struct		mm;		/* Memory blocks */
    pid_t			pgrp;
    struct tty			*tty;
    struct task_struct		*p_parent;
#if BLOAT
    struct task_struct		*p_prevsib;
    struct task_struct		*p_nextsib;
    struct task_struct		*p_child;
#endif
    struct wait_queue		child_wait;
    int				exit_status;	/* process exit status*/
    struct inode		*t_inode;
    sigset_t			signal;		/* Signal status */
    struct signal_struct	sig;		/* Signal block */

#ifdef CONFIG_SUPPLEMENTARY_GROUPS
    gid_t			groups[NGROUPS];
#endif

#ifdef CONFIG_STRACE
    struct syscall_params	sc_info;
#endif

    __u16			t_kstackm;	/* To detect stack corruption */
    __u8			t_kstack[KSTACK_BYTES];
    __registers 		t_regs;
};

#define KSTACK_MAGIC 0x5476

/* the order of these matter for signal handling*/
#define TASK_RUNNING 		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE 	2
#define TASK_WAITING		3
#define TASK_STOPPED		4
#define TASK_ZOMBIE		5
#define TASK_EXITING		6
#define TASK_UNUSED		7

/*@-namechecks@*/

#define DEPRECATED
//#define DEPRECATED	__attribute__ ((deprecated))

/* We use typedefs to avoid using struct foobar (*) */
typedef struct task_struct __task, *__ptask;

extern __task task[MAX_TASKS];

extern volatile jiff_t jiffies;	/* ticks updated by the timer interrupt*/
extern __ptask current;
extern int need_resched;

extern struct timeval xtime;
extern jiff_t xtime_jiffies;
#define CURRENT_TIME (xtime.tv_sec + (jiffies - xtime_jiffies)/HZ)

#define for_each_task(p) \
	for (p = &task[0] ; p!=&task[MAX_TASKS]; p++ )

/* Scheduling and sleeping function prototypes */

extern void schedule(void);

extern void wait_set(struct wait_queue *);
extern void wait_clear(struct wait_queue *);

/*
 * Using sleep_on allows a race condition which in certain circumstances
 * may lose a wake_up between the condition check and sleep_on/schedule.
 * Only wait queues or condition checks changed by hw interrupts need
 * use race-safe sleep calls.
 */
extern void sleep_on(struct wait_queue *) DEPRECATED;
extern void interruptible_sleep_on(struct wait_queue *) DEPRECATED;

/* simple race-safe sleep calls */
void prepare_to_wait_interruptible(struct wait_queue *p);
void prepare_to_wait(struct wait_queue *p);
void do_wait(void);
void finish_wait(struct wait_queue *p);

/*@-namechecks@*/

extern void _wake_up(struct wait_queue *,unsigned short int);

/*@+namechecks@*/

extern void down(short *);
extern void up(short *);

extern void wake_up_process(struct task_struct *);

extern int kill_process(pid_t,sig_t,int);
extern void kill_all(sig_t);

extern void add_to_runqueue(struct task_struct *);

extern struct task_struct *find_empty_process(void);
extern void arch_build_stack(struct task_struct *, char *);
extern int restart_syscall(void);
extern unsigned int get_ustack(struct task_struct *,int);
extern void put_ustack(register struct task_struct *,int,int);

extern void tswitch(void);

void select_wait(struct wait_queue *);
int select_poll(struct task_struct *, struct wait_queue *);

#endif
