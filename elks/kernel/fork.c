#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/debug.h>

int check_task_table(void)
{
    register int i;
    register int j;
    j = 0;
    for (j = 0; j < MAX_TASKS; j++) {
	if (task[i].state != TASK_UNUSED && task[i].pid == 8) {
	    j++;
	}
    }
    return j;
}

/*
 *	Find a free task slot.
 */
static int find_empty_process(void)
{
    register int i;
    register int unused = 0;
    int n;

    for (i = 0; i < MAX_TASKS; i++) {
	if (task[i].state == TASK_UNUSED) {
	    unused++;
	    n = i;
	}
    }

    if (unused <= 1)
	printk("Only %d slots\n", unused);

    if (unused == 0 || (unused == 1 && current->uid))
	return -EAGAIN;

    return n;
}


int get_pid(void)
{
    static unsigned int last_pid = 0;
    register struct task_struct *p;
    register int i;

  repeat:
    if (++last_pid == 32768)
	last_pid = 1;

    for (i = 0; i < MAX_TASKS; i++) {
	p = &task[i];
	if (p->pid == last_pid || p->pgrp == last_pid ||
	    p->session == last_pid) goto repeat;
    }
    return last_pid;
}

/*
 *	Clone a process.
 */

pid_t do_fork(int virtual)
{
    register struct task_struct *t;
    int i = find_empty_process(), j;
    struct file *filp;
    register __ptask currentp = current;

    if (i < 0)
	return i;

    t = &task[i];

    /* Copy everything */

    *t = *currentp;

    /* Fix up what's different */

    /* 
     * We do shared text.
     */
    mm_realloc(currentp->mm.cseg);

    if (virtual) {
	mm_realloc(currentp->mm.dseg);
    } else {
	t->mm.dseg = mm_dup(currentp->mm.dseg);
	if (t->mm.dseg == NULL) {
	    mm_free(currentp->mm.cseg);
	    t->state = TASK_UNUSED;
	    return -ENOMEM;
	}

	t->t_regs.ds = t->mm.dseg;
	t->t_regs.ss = t->mm.dseg;
    }
    t->t_regs.ksp = t->t_kstack + KSTACK_BYTES;
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
    t->p_nextsib = NULL;
    t->p_child = NULL;
    t->child_lastend = 0;
    t->lastend_status = 0;
    if (currentp->p_child) {
	currentp->p_child->p_nextsib = t;
	t->p_prevsib = currentp->p_child;
    } else {
	t->p_prevsib = NULL;
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
	unsigned ip, cs, fl;
	ip = get_ustack(currentp, 6);
	cs = get_ustack(currentp, 2);
	fl = get_ustack(currentp, 4);
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
    return do_fork(1);
}
