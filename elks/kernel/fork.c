#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/trace.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>

int task_slots_unused;
struct task_struct *next_task_slot;
pid_t last_pid = -1;

static pid_t get_pid(void)
{
    register struct task_struct *p;

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
    } while (++p < &task[max_tasks]);
    return last_pid;
}

/*
 *  Find a free task slot.
 */
struct task_struct *find_empty_process(void)
{
    register struct task_struct *t;

    if (task_slots_unused <= 1) {
        printk("Only %d task slots\n", task_slots_unused);
        if (!task_slots_unused || current->uid)
            return NULL;
    }
    t = next_task_slot;
    while (t->state != TASK_UNUSED) {
        if (++t >= &task[max_tasks])
            t = &task[1];
    }
    next_task_slot = t;
    task_slots_unused--;
    *t = *current;
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
    int j, k;
    struct file *filp;
    struct segment *s;

    if ((t = find_empty_process()) == NULL)
        return -EAGAIN;
    debug_wait("FORK(%P): -> %d\n", t->pid);

    /* Fix up what's different */

    /* t->mm[] is a duplicate of current->mm[] by find_empty_process */
    for (j = 0; j < MAX_SEGS; j++) {
        s = t->mm[j];
        if (s) {
            if ((s->flags & SEG_FLAG_TYPE) == SEG_FLAG_CSEG)
                seg_get(s);         /* share text */
            else {
                if (virtual) {
                    seg_get(s);     /* share data for vfork */
                } else {
                    t->mm[j] = seg_dup(s);
                    if (t->mm[j] == 0) {
                        for (k = 0; k < j; k++)
                            seg_put(t->mm[k]);
                        t->state = TASK_UNUSED;
                        task_slots_unused++;
                        next_task_slot = t;
                        return -ENOMEM;
                    }

                    if ((t->mm[j]->flags & SEG_FLAG_TYPE) == SEG_FLAG_DSEG)
                        t->t_regs.ds = t->t_regs.es = t->t_regs.ss = t->mm[j]->base;
                }
            }
        }
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

    t->ppid = current->pid;
    t->p_parent = current;

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
        memcpy_fromfs(sc, (void *)current->t_regs.sp, sizeof(sc));
        /*
         * Let the child go on first.
         */
        sleep_on(&current->child_wait);
        /*
         * By now, the child should have its own user stack. Restore
         * the parent's user stack.
         */
        memcpy_tofs((void *)current->t_regs.sp, sc, sizeof(sc));
    }
    return retval;
#endif
}
