/* linuxmt/kernel/exit.c
 * (C) 1997 Chad Page
 * 
 * This is the ELKS code to handle wait (partial V7-style implementation),
 * and sys_exit().  I've thrown this together to handle mini-sh so I can 
 * get this to 0.(0?)1.0 :)
 */

#include <linuxmt/sched.h>
#include <linuxmt/errno.h>

/* Note : sys_wait only keeps *one* task in the task_struct right now...
 * this is different than V7 symantics I think, but good enough for 0.0.51
 * Whoops - we have to do wait3 for now :) 
 */

pid_t sys_wait4(pid, status, options)
pid_t pid;
register int * status;
int options;
{
	pid_t retval;
	register pid_t * lastendp = &current->child_lastend;

	if (!((options & WNOHANG) || (*lastendp))) {
		interruptible_sleep_on(&current->child_wait);
	}
	if (*lastendp) {
		if (status) {
			if ((retval = verified_memcpy_tofs(status, 
					&current->lastend_status, 
					sizeof (int))) != 0)
				return retval;
		}
		retval = *lastendp;
		*lastendp = 0;
		return retval;
	}
	return -EINTR;
}

void do_exit(status)
int status;
{
	register __ptask parent;
	register __ptask task;

	_close_allfiles();	
	/* Let go of the process */
	current->state = TASK_EXITING;
	if (current->mm.cseg)  
		mm_free(current->mm.cseg);
	if (current->mm.dseg)
		mm_free(current->mm.dseg);
	current->mm.cseg = NULL;
	current->mm.dseg = NULL;
	/* Keep all of the family stuff straight */
	if (task = current->p_prevsib) {
		task->p_nextsib = current->p_nextsib;
	}
	if (task = current->p_nextsib) {
		task->p_prevsib = current->p_prevsib;
	}
	/* Ack. I hate repeating code like this */
	parent = current->p_parent;
	if (parent->p_child == current) {
		if (task = current->p_prevsib) { 
			parent->p_child = task;
		} else 
		if (task = current->p_nextsib) {
			parent->p_child = task;
		}
	}
	/* UN*X process take their children out with them...
	 * I'm not going to implement that for 0.0.51 becuase we don't 
	 * have signals and we don't need them *yet*. */

	/* Let the parent know */
	kill_process(current->ppid, SIGCHLD, 1);

	/* Send control back to the parent */
	parent->child_lastend = current->pid;
	parent->lastend_status = status;
	/* Free the pwd and root inodes */
	iput(current->fs.root);
	iput(current->fs.pwd);
	/* Now the task should never run again... - I hope this can still
 	 * be used outside of an int... :) */
	current->state = TASK_UNUSED;
	wake_up(&parent->child_wait);
	schedule();
	panic("Returning from sys_exit!\n");
}

void sys_exit(status)
int status;
{
	status = (status & 0xff) << 8;
	do_exit(status);
}
