/* linuxmt/kernel/exit.c
 * (C) 1997 Chad Page
 * 
 * This is the ELKS code to handle wait (partial V7-style implementation),
 * and sys_exit().  I've thrown this together to handle mini-sh so I can 
 * get this to 0.(0?)1.0 :)
 */

#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>

extern int task_slots_unused;
extern struct task_struct *next_task_slot;

/* Note: sys_wait only keeps *one* task in the task_struct right now...
 * this is different than V7 symantics I think, but good enough for 0.0.51
 * Whoops - we have to do wait3 for now :) 
 */

int sys_wait4(pid_t pid, int *status, int options)
{
    register __ptask p = current;
    register pid_t *lastendp = &p->child_lastend;

    if (!((options & WNOHANG) || (*lastendp)))
	interruptible_sleep_on(&p->child_wait);
    if (*lastendp) {
	if (status) {
	    if (verified_memcpy_tofs(status,
				     &p->lastend_status,
				     sizeof(int)) != 0) {
		return -EFAULT;
	    }
	}
	options = *lastendp;	/* TRASHING options!!! */
	*lastendp = 0;
	return options;
    }
    return -EINTR;
}

void do_exit(int status)
{
    register __ptask pcurrent = current;
#define current pcurrent
     __ptask parent;
    register __ptask task;

    _close_allfiles();

    /* Let go of the process */
    current->state = TASK_EXITING;
    if (current->mm.cseg)
	mm_free(current->mm.cseg);
    if (current->mm.dseg)
	mm_free(current->mm.dseg);

    current->mm.cseg = current->mm.dseg = 0;

    /* Keep all of the family stuff straight */
    if ((task = current->p_prevsib)) {
	task->p_nextsib = current->p_nextsib;
    }
    if ((task = current->p_nextsib)) {
	task->p_prevsib = current->p_prevsib;
    }

    /* Ack. I hate repeating code like this */
#if 0
    parent = current->p_parent;
    if (parent->p_child == current) {
	if ((task = current->p_prevsib)) {
	    parent->p_child = task;
	} else if ((task = current->p_nextsib)) {
	    parent->p_child = task;
	}
    }
#else
    if ((parent = current->p_parent)->p_child == current) {
	if ((task = current->p_prevsib)
	    || (task = current->p_nextsib)
	    ) {
	    parent->p_child = task;
	}
    }

    /* Ok... we're done with task, but we still need parent a few
     * times.  Since task is a register and parent isn't, stuff
     * parent in task and use that. */
    task = parent;
#define parent task

#endif

    /* UN*X process take their children out with them...
     * I'm not going to implement that for 0.0.51 because we don't
     * have signals and we don't need them *yet*. */

    /* Let the parent know */
    kill_process(current->ppid, SIGCHLD, 1);

    /* Send control back to the parent */
    parent->child_lastend = current->pid;
    parent->lastend_status = status;

    /* Free the text, pwd, and root inodes */
    iput(current->t_inode);
    iput(current->fs.root);
    iput(current->fs.pwd);

    /* Now the task should never run again... - I hope this can still
     * be used outside of an int... :) */
    current->state = TASK_UNUSED;
    next_task_slot = current;
    task_slots_unused++;
    wake_up(&parent->child_wait);
    schedule();
    panic("Returning from sys_exit!\n");
}
#undef current

void sys_exit(int status)
{
    do_exit((status & 0xff) << 8);
}
