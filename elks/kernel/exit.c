/* linuxmt/kernel/exit.c
 * (C) 1997 Chad Page
 * 
 * Handle sys_wait4() and sys_exit().
 */

#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>

extern int task_slots_unused;
extern struct task_struct *next_task_slot;

int sys_wait4(pid_t pid, int *status, int options)
{
	register struct task_struct *p;
	struct task_struct *q;

	debug_wait("WAIT(%d) for %d %s\n", current->pid, pid, (options & WNOHANG)? "nohang": "");

	/* reparent orphan zombies to init*/
	for_each_task(p) {
		if (p->state == TASK_ZOMBIE) {
			debug_wait("Zombie pid %d ppid %d\n", p->pid, p->p_parent->pid);
			if (p->p_parent->state == TASK_UNUSED) {
				debug_wait("WAIT(%d) reparenting %d to 1\n", current->pid, p->pid);
				p->p_parent = &task[1];
				wake_up(&task[1].child_wait);
			}

		}
	}

	for_each_task(p) {
		if (p->p_parent == current
		   && (p->state == TASK_ZOMBIE || p->state == TASK_STOPPED)) {
			if (pid == -1 || p->pid == pid || (!pid && p->pgrp == current->pgrp)) {
				if (status) {
					if (verified_memcpy_tofs(status, &p->exit_status, sizeof(int)))
						return -EFAULT;
				}

				/* just return status on stopped state, don't release task*/
				if (p->state == TASK_STOPPED)
					return p->pid;

				/* must reparent orphans before unassigning task slot*/
				for_each_task(q) {
					if (q->p_parent == p) {
						debug_wait("Orphan child %d\n", q->pid);
						q->p_parent = &task[1];
					}
				}

				/* unassign now that no orphans pointing to it*/
				p->state = TASK_UNUSED;
				next_task_slot = p;
				task_slots_unused++;

				debug_wait("WAIT(%d) got %d\n", current->pid, p->pid);
				return p->pid;
			}
		}
	}

	if (options & WNOHANG)
		return 0;

	debug_wait("WAIT sleep\n");
	interruptible_sleep_on(&current->child_wait);
	debug_wait("WAIT wakeup\n");

	return -EINTR;
}

void do_exit(int status)
{
    struct task_struct *parent, *task;

    debug_wait("EXIT(%d) status %d\n", current->pid, status);
    _close_allfiles();

    /* release process group and TTY*/
    current->pgrp = 0;
    current->tty = 0;

    /* Let go of the process */
    current->state = TASK_EXITING;
    if (current->mm.seg_code)
	seg_put(current->mm.seg_code);
    if (current->mm.seg_data)
	seg_put(current->mm.seg_data);

    current->mm.seg_code = current->mm.seg_data = 0;

    /* Keep all of the family stuff straight */
    if ((task = current->p_prevsib)) {
	task->p_nextsib = current->p_nextsib;
    }
    if ((task = current->p_nextsib)) {
	task->p_prevsib = current->p_prevsib;
    }

    /* Ack. I hate repeating code like this */
    if ((parent = current->p_parent)->p_child == current) {
	if ((task = current->p_prevsib) || (task = current->p_nextsib))
	    parent->p_child = task;
    }

    /* UN*X process take their children out with them...
     * I'm not going to implement that for 0.0.51 because we don't
     * have signals and we don't need them *yet*. */

    current->exit_status = status;

    /* Let the parent know */
    kill_process(current->ppid, SIGCHLD, 1);

    /* Free the text, pwd, and root inodes */
    iput(current->t_inode);
    iput(current->fs.root);
    iput(current->fs.pwd);

    /* Now the task should never run again... - I hope this can still
     * be used outside of an int... :) */
    current->state = TASK_ZOMBIE;
    wake_up(&parent->child_wait);
    schedule();
    panic("sys_exit\n");
}

void sys_exit(int status)
{
    do_exit((status & 0xff) << 8);
}
