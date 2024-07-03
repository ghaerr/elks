#ifndef __LINUXMT_SCHED_H
#define __LINUXMT_SCHED_H

#include <linuxmt/config.h>
#include <linuxmt/limits.h>
#include <linuxmt/types.h>
#include <linuxmt/fs.h>
#include <linuxmt/time.h>
#include <linuxmt/signal.h>
#include <linuxmt/wait.h>
#include <linuxmt/ntty.h>
#include <arch/param.h>

struct file_struct {
    fd_mask_t                   close_on_exec;
    struct file                 *fd[NR_OPEN];
};

struct fs_struct {
    unsigned short int          umask;
    struct inode                *root;
    struct inode                *pwd;
};

struct signal_struct {
    __kern_sighandler_t         handler;
    struct __kern_sigaction_struct action[NSIG];
};

/* Standard segment entry indices for a.out executables */
#define SEG_CODE        0
#define SEG_DATA        1

struct task_struct {

/* Executive stuff */
    struct xregs                t_xregs;    /* CS and kernel SP */
    segoff_t                    t_enddata;  /* start of heap = end of data+bss */
    segoff_t                    t_endbrk;   /* current break (end of heap) */
    segoff_t                    t_begstack; /* start SP, argc/argv strings above */
    segoff_t                    t_endseg;   /* end of data seg (data+bss+heap+stack) */
    segoff_t                    t_minstack; /* min stack size */

/* Kernel info */
    pid_t                       pid;
    pid_t                       ppid;
    pid_t                       pgrp;
    pid_t                       session;
    uid_t                       uid;
    uid_t                       euid;
    uid_t                       suid;
    gid_t                       gid;
    gid_t                       egid;
    gid_t                       sgid;

/* Scheduling + status variables */
    unsigned char               state;
    struct wait_queue           child_wait;
    jiff_t                      timeout;        /* for select() */
    struct wait_queue           *waitpt;        /* Wait pointer */
    struct wait_queue           *poll[POLL_MAX];  /* polled queues */
    struct task_struct          *next_run;
    struct task_struct          *prev_run;
    struct file_struct          files;          /* File system structure */
    struct fs_struct            fs;             /* File roots */
    struct segment              *mm[MAX_SEGS];  /* App code/data segments */
    struct tty                  *tty;
    struct task_struct          *p_parent;
    int                         exit_status;    /* process exit status*/
    struct inode                *t_inode;
    sigset_t                    signal;         /* Signal status */
    struct signal_struct        sig;            /* Signal block */

#ifdef CONFIG_CPU_USAGE
    unsigned long               average;        /* fixed point CPU % usage */
    unsigned char               ticks;          /* # jiffies / 2 seconds */
#endif

#ifdef CONFIG_SUPPLEMENTARY_GROUPS
#define NGROUPS     13
    gid_t                       groups[NGROUPS];
#endif

    /* next two words are only used by CONFIG_TRACE but left in to avoid
     * changing struct task size and having to recompile 'ps' etc when changed */
    int                         kstack_max;
    int                         kstack_prevmax;

    unsigned int                kstack_magic;   /* To detect stack corruption */
    __u16                       t_kstack[KSTACK_BYTES/2];
    struct pt_regs              t_regs;         /* registers on stack during syscall */
};

#define KSTACK_MAGIC 0x5476

/* the order of these matter for signal handling*/
#define TASK_RUNNING            0
#define TASK_INTERRUPTIBLE      1
#define TASK_UNINTERRUPTIBLE    2
#define TASK_WAITING            3
#define TASK_STOPPED            4
#define TASK_ZOMBIE             5
#define TASK_EXITING            6
#define TASK_UNUSED             7

#define DEPRECATED
//#define DEPRECATED    __attribute__ ((deprecated))

extern struct task_struct *task;
extern struct task_struct *current;
extern struct task_struct *next_task_slot;
extern int max_tasks;
extern int task_slots_unused;

extern volatile jiff_t jiffies; /* ticks updated by the timer interrupt*/
extern pid_t last_pid;
extern int intr_count;

extern struct timeval xtime;
extern jiff_t xtime_jiffies;
extern int tz_offset;
extern time_t current_time(void);

/* return true if time a is after time b */
#define time_after(a,b)         (((long)(b) - (long)(a) < 0))

#define for_each_task(p) \
        for (p = &task[0] ; p!=&task[max_tasks]; p++ )

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

extern void _wake_up(struct wait_queue *,int);

extern void down(sem_t *);
extern void up(sem_t *);

extern void wake_up_process(struct task_struct *);

extern int kill_process(pid_t,sig_t,int);
extern void kill_all(sig_t);

extern void add_to_runqueue(struct task_struct *);

extern struct task_struct *find_empty_process(void);
extern void arch_build_stack(struct task_struct *, void (*)());
extern int restart_syscall(void);
extern unsigned int get_ustack(struct task_struct *,int);
extern void put_ustack(register struct task_struct *,int,int);

extern void tswitch(void);
extern void setsp(void *);
extern int run_init_process(const char *cmd);
extern int run_init_process_sptr(const char *cmd, char *sptr, int slen);
extern void ret_from_syscall(void);
extern void check_stack(void);

void select_wait(struct wait_queue *);
int select_poll(struct task_struct *, struct wait_queue *);

#endif
