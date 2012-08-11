#include <linuxmt/config.h>
#include <linuxmt/debug.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>

int task_slots_unused = MAX_TASKS - 2;
struct task_struct *next_task_slot = &task[2];

/*
 *	Find a free task slot.
 */
static struct task_struct *find_empty_process(void)
{
    register struct task_struct *t;

    if (task_slots_unused <= 1) {
        printk("Only %d slots\n", task_slots_unused);
        if (!task_slots_unused || current->uid)
            return NULL;
	}
    t = next_task_slot;
    while (t->state != TASK_UNUSED) {
        if (++t >= &task[MAX_TASKS])
            t = &task[1];
    }
    next_task_slot = t;
    task_slots_unused--;
    return t;
}


pid_t get_pid(void)
{
    register struct task_struct *p;
    static pid_t last_pid = 0;

repeat:
    if (++last_pid < 0)
        last_pid = 1;
                
    p = &task[0];
    do {
        if (p->state == TASK_UNUSED)
            continue;
	if (p->pid == last_pid || p->pgrp == last_pid ||
	    p->session == last_pid) {
	    goto repeat;
	}
    } while (++p < &task[MAX_TASKS]);
    return last_pid;
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

    if((t = find_empty_process()) == NULL)
        return -EAGAIN;

    /* Copy everything */

    *t = *currentp;

    /* Fix up what's different */

    /* 
     * We do shared text.
     */
    (void) mm_realloc(currentp->mm.cseg);

    if (virtual) {
	(void) mm_realloc(currentp->mm.dseg);
    } else {
	t->mm.dseg = mm_dup(currentp->mm.dseg);

	if (t->mm.dseg == NULL) {
	    mm_free(currentp->mm.cseg);
	    t->state = TASK_UNUSED;
            task_slots_unused++;
            next_task_slot = t;
	    return -ENOMEM;
	}

	t->t_regs.ds = t->t_regs.ss = t->mm.dseg;
    }
    t->t_regs.ksp = ((__u16) t->t_kstack) + KSTACK_BYTES;

    t->state = TASK_UNINTERRUPTIBLE;
    t->pid = get_pid();
    t->ppid = currentp->pid;
    t->t_kstackm = KSTACK_MAGIC;
    t->next_run = t->prev_run = NULL;

    /*
     *      Build a return stack for t.
     */

    arch_build_stack(t);

    /* Increase the reference count to all open files */

    for (j = 0; j < NR_OPEN; j++)
	if ((filp = currentp->files.fd[j]))
	    filp->f_count++;

    /* Increase the reference count for program text inode - tgm */
    t->t_inode->i_count++;
    t->fs.root->i_count++;
    t->fs.pwd->i_count++;

    /* Set up our family tree */
    t->p_parent = currentp;
    t->p_nextsib = t->p_child = NULL;
    t->child_lastend = t->lastend_status = 0;
    if ((t->p_prevsib = currentp->p_child) != NULL) {
	currentp->p_child->p_nextsib = t;
    }
    currentp->p_child = t;

    /*
     *      Wake our new process
     */
    wake_up_process(t);

    /*
     *      Return the created task.
     */
    if (virtual) {
	/* When the parent returns, the stack frame will have gone so
	 * save enough stack state that we will be able to return to
	 * the point where vfork() was called in the code, not to the
	 * point in the library code where the actual syscall int was
	 * done - ajr 16th August 1999
	 *
	 * Stack was
	 *         ip cs f ret
	 * and will be
	 *            ret cs f
	 */
	int ip, cs, fl;

	ip = (int) get_ustack(currentp, 6);
	cs = (int) get_ustack(currentp, 2);
	fl = (int) get_ustack(currentp, 4);
	currentp->t_regs.sp += 2;
	sleep_on(&currentp->child_wait);
	put_ustack(currentp, 0, ip);
	put_ustack(currentp, 2, cs);
	put_ustack(currentp, 4, fl);

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
