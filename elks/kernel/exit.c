/* linuxmt/kernel/exit.c
 * (C) 1997 Chad Page
 * 
 * Handle sys_wait4() and sys_exit().
 */

#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>

static void reparent_children(void)
{
    register struct task_struct *p;

    for_each_task(p) {
        if (p->p_parent == current) {
            /* remove orphaned zombies, no need to reparent them to init*/
            if (p->state == TASK_ZOMBIE) {
                debug_wait("Zombie orphan pid %d ppid %d removed\n", p->pid, p->ppid);
                p->state = TASK_UNUSED;     /* unassign task entry*/
                next_task_slot = p;
                task_slots_unused++;
            }

            /* reparent orphans to init*/
            if (p->state != TASK_UNUSED) {
                debug_wait("Reparenting orphan pid %d ppid %d to init\n",
                    p->pid, p->p_parent->pid);
                p->p_parent = &task[1];
                p->ppid = task[1].pid;
            }
        }
    }
}

/* note: 'usage' parameter ignored */
int sys_wait4(pid_t pid, int *status, int options, void *usage)
{
    register struct task_struct *p;
    int waitagain;

    debug_wait("WAIT(%P) for %d %s\n", pid, (options & WNOHANG)? "nohang": "");

 for (;;) {
    waitagain = 0;

    for_each_task(p) {
        if (p->p_parent == current && p->state != TASK_UNUSED) {
          if (p->state == TASK_ZOMBIE || p->state == TASK_STOPPED) {
            if (pid == (pid_t)-1 || p->pid == pid || (!pid && p->pgrp == current->pgrp)) {
                if (status) {
                    if (verified_memcpy_tofs(status, &p->exit_status, sizeof(int)))
                        return -EFAULT;
                }

                /* just return status on stopped state, don't release task*/
                if (p->state == TASK_STOPPED)
                    return p->pid;

                p->state = TASK_UNUSED;     /* unassign task entry*/
                next_task_slot = p;
                task_slots_unused++;

                debug_wait("WAIT(%P) got %d\n", p->pid);
                return p->pid;
            }
        } else {
            /* keep waiting while process has non-zombie/stopped children*/
            debug_wait("WAIT(%P) again for pid %d state %d\n", p->pid, p->state);
            waitagain = 1;
        }
      }
    }

    if (options & WNOHANG)
        return 0;
    if (!waitagain)
        break;

    debug_wait("WAIT(%P) sleep\n");
    interruptible_sleep_on(&current->child_wait);
    if (current->signal) {
        debug_wait("WAIT(%P) return -EINTR\n");
        return -EINTR;
    }
    debug_wait("WAIT(%P) wakeup\n");
  }

    debug_wait("WAIT(%P) return -ECHILD\n");
    return -ECHILD;
}

void do_exit(int status)
{
    int i;
    struct task_struct *parent;

    debug_wait("EXIT(%P) status %d\n", status);
    _close_allfiles();

    /* release process group and TTY*/
    current->pgrp = 0;
    current->tty = 0;

    /* Let go of the process */
    current->state = TASK_EXITING;
    for (i = 0; i < MAX_SEGS; i++) {
        if (current->mm[i])
            seg_put(current->mm[i]);
        current->mm[i] = 0;
    }

    /* free program allocated memory */
    seg_free_pid(current->pid);

    parent = current->p_parent;

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

    /* remove orphan zombies and reparent children to init*/
    reparent_children();

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
