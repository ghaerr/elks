#include <linuxmt/config.h>
#include <linuxmt/debug.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <arch/segment.h>

int task_slots_unused = MAX_TASKS;
struct task_struct *next_task_slot = task;

pid_t get_pid(void)
{
    register struct task_struct *p;
    static pid_t last_pid = -1;

    goto startgp;
    do {
        if (p->state == TASK_UNUSED)
            continue;
	if (p->pid == last_pid || p->pgrp == last_pid ||
	    p->session == last_pid) {
	  startgp:
	    if (++last_pid < 0)
		last_pid = 1;
	    p = &task[0];
	}
    } while (++p < &task[MAX_TASKS]);
    return last_pid;
}

/*
 *	Find a free task slot.
 */
struct task_struct *find_empty_process(void)
{
    register struct task_struct *t;
    register struct task_struct *currentp = current;

    if (task_slots_unused <= 1) {
        printk("Only %d slots\n", task_slots_unused);
        if (!task_slots_unused || currentp->uid)
            return NULL;
    }
    t = next_task_slot;
    while (t->state != TASK_UNUSED) {
        if (++t >= &task[MAX_TASKS])
            t = &task[1];
    }
    next_task_slot = t;
    task_slots_unused--;
    *t = *currentp;
    t->state = TASK_UNINTERRUPTIBLE;
    t->pid = get_pid();
    t->t_kstackm = KSTACK_MAGIC;
    t->next_run = t->prev_run = NULL;
    return t;
}

/*
 *	Clone a process.
 */

pid_t do_fork(int virtual)
{
    register struct task_struct *t;
    pid_t j;
    struct file *filp;
    register __ptask currentp = current;
    int sc[4];

    if((t = find_empty_process()) == NULL)
        return -EAGAIN;

    /* Fix up what's different */

    /*
     * We do shared text.
     */
    mm_realloc(currentp->mm.cseg);

    if (virtual) {
	mm_realloc(currentp->mm.dseg);
    } else {
	t->mm.dseg = mm_dup(currentp->mm.dseg);

	if (t->mm.dseg == 0) {
	    mm_free(currentp->mm.cseg);
	    t->state = TASK_UNUSED;
            task_slots_unused++;
            next_task_slot = t;
	    return -ENOMEM;
	}

	t->t_regs.ds = t->t_regs.ss = t->mm.dseg;
    }

    /* Increase the reference count to all open files */

    j = 0;
    do {
	if ((filp = t->files.fd[j]))
	    filp->f_count++;
    } while(++j < NR_OPEN);

    /* Increase the reference count for program text inode - tgm */
    t->t_inode->i_count++;
    t->fs.root->i_count++;
    t->fs.pwd->i_count++;

    /* Set up our family tree */
    t->ppid = currentp->pid;
    t->p_parent = currentp;
    t->p_nextsib = t->p_child = NULL;
    t->child_lastend = t->lastend_status = 0;
    if ((t->p_prevsib = currentp->p_child) != NULL) {
	currentp->p_child->p_nextsib = t;
    }
    currentp->p_child = t;

    /*
     *      Build a return stack for t.
     */
    arch_build_stack(t, NULL);
    /* Wake our new process */
    wake_up_process(t);

    if (virtual) {

	/* Parent and child are sharing the user stack at this point.
	 * The child will go first, returning from this function and
	 * from the library code where the actual syscall was done
	 * and then will issue an exec syscall, destroying the first
	 * few bytes at the top of the user stack. Save those bytes
	 * in the parent's kernel stack.
	 */
	fmemcpy(kernel_ds, sc, currentp->t_regs.ss, currentp->t_regs.sp, 8);
	/*
	 * Let the child go on first.
	 */
	sleep_on(&currentp->child_wait);
	/*
	 * By now, the child should have its own user stack. Restore
	 * the parent's user stack.
	 */
	fmemcpy(currentp->t_regs.ss, currentp->t_regs.sp, kernel_ds, sc, 8);
    }
    /*
     *      Return the created task.
     */
    return t->pid;
}

pid_t sys_fork(void)
{
    return do_fork(0);
}

pid_t sys_vfork(void)
{
#ifdef CONFIG_EXEC_ELKS
    return do_fork(0);
#else
    return do_fork(1);
#endif
}
