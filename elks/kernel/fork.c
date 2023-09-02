#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/trace.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>

int task_slots_unused = MAX_TASKS;
struct task_struct *next_task_slot = task;

static pid_t get_pid(void)
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
            if ((int)(++last_pid) < 0)
                last_pid = 1;
            p = &task[0];
        }
    } while (++p < &task[MAX_TASKS]);
    return last_pid;
}

/*
 *  Find a free task slot.
 */
struct task_struct *find_empty_process(void)
{
    register struct task_struct *t;
    register struct task_struct *currentp = current;

    if (task_slots_unused <= 1) {
        printk("Only %d task slots\n", task_slots_unused);
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
#ifdef CONFIG_CPU_USAGE
    t->ticks = 0;
    t->average = 0;
#endif
#ifdef CHECK_KSTACK
    t->kstack_max = 0;
    t->kstack_prevmax = 0;
#endif
    t->kstack_magic = KSTACK_MAGIC;
    t->next_run = t->prev_run = NULL;
    return t;
}

/*
 *  Clone a process.
 */

pid_t do_fork(int virtual)
{
    register struct task_struct *t;
    int j;
    struct file *filp;
    register __ptask currentp = current;

    if ((t = find_empty_process()) == NULL)
        return -EAGAIN;

    /* Fix up what's different */

    /*
     * We do shared text.
     */
    seg_get(currentp->mm.seg_code);

    if (virtual) {
        seg_get(currentp->mm.seg_data);
    } else {
        t->mm.seg_data = seg_dup(currentp->mm.seg_data);

        if (t->mm.seg_data == 0) {
            seg_put (currentp->mm.seg_code);
            t->state = TASK_UNUSED;
            task_slots_unused++;
            next_task_slot = t;
            return -ENOMEM;
        }

        t->t_regs.ds = t->t_regs.es = t->t_regs.ss = (t->mm.seg_data)->base;
    }

    /* Increase the reference count to all open files */

    j = 0;
    do {
        if ((filp = t->files.fd[j]))
            filp->f_count++;
    } while (++j < NR_OPEN);

    /* Increase the reference count for program text inode - tgm */
    t->t_inode->i_count++;
    t->fs.root->i_count++;
    t->fs.pwd->i_count++;

    t->exit_status = 0;

    t->ppid = currentp->pid;
    t->p_parent = currentp;

    /*
     *      Build a return stack for t.
     */
    arch_build_stack(t, NULL);
    /* Wake our new process */
    wake_up_process(t);

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
#if 1
    return do_fork(0);
#else
    register __ptask currentp = current;
    int retval, sc[5];

    if ((retval = do_fork(1)) >= 0) {

        /* Parent and child are sharing the user stack at this point.
         * The child will go first, coming into life in the middle of
         * the tswitch() function, returning to user space, then will
         * return from the library code where the actual syscall was
         * done and then will issue an exec syscall, destroying the
         * first few bytes at the top of the user stack. Save those
         * bytes in the parent's kernel stack.
         */
        memcpy_fromfs(sc, (void *)currentp->t_regs.sp, sizeof(sc));
        /*
         * Let the child go on first.
         */
        sleep_on(&currentp->child_wait);
        /*
         * By now, the child should have its own user stack. Restore
         * the parent's user stack.
         */
        memcpy_tofs((void *)currentp->t_regs.sp, sc, sizeof(sc));
    }
    return retval;
#endif
}
