#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/debug.h>

/*
 *	Find a free task slot.
 */

static int find_empty_process()
{
	int i;
	int unused=0;
	int n;
	
	for(i=0;i<MAX_TASKS;i++)
	{
		if(task[i].state==TASK_UNUSED)
		{
			unused++;
			n=i;
		}
	}

	if (unused <=1) printk("Only %d slots\n", unused);	
	if(unused==0 || (unused==1 && current->uid))
		return -EAGAIN;
	
	return n;
}


int get_pid()
{
	static unsigned int last_pid = 0;
	register struct task_struct *p;
	int i;
	
repeat:
	if(++last_pid==32768)
		last_pid=1;
			
	for(i=0;i<MAX_TASKS;i++)
	{
		p=&task[i];
		if(p->pid==last_pid || p->pgrp==last_pid || 
			p->session == last_pid)
			goto repeat;
	}
	return last_pid;
}

/*
 *	Clone a process.
 */
 
int do_fork()
{
	register struct task_struct *t;
	int i=find_empty_process(), j;
	register struct file * filp;

	if(i<0)
		return i;

	t=&task[i];
	
	/*
	 *	Copy everything
	 */
	 
	*t = *current;
	
	/*
	 *	Fix up what's different
	 */

	/* We can now do shared text with fork using realloc */
	t->mm.cseg = mm_realloc(current->mm.cseg);
/* 	if (t->mm.cseg == -1) {
		return -ENOMEM;
	} */ /* mm_realloc Cannot return -1 ever. */
	
	t->t_regs.cs = t->mm.cseg;
 
	t->mm.dseg = mm_dup(current->mm.dseg);
	if (t->mm.dseg == -1) {
		return -ENOMEM;
	}
	t->t_regs.ds = t->mm.dseg;
	t->t_regs.ss = t->mm.dseg;

	t->t_regs.ksp=t->t_kstack+KSTACK_BYTES;
	t->state = TASK_UNINTERRUPTIBLE;
	t->pid = get_pid();
	t->ppid = current->pid;
	t->t_kstackm = KSTACK_MAGIC;
	t->next_run = t->prev_run = NULL;
	
	/*
	 *	Build a return stack for t.
	 */
	 
	arch_build_stack(t);

	/* Increase the reference count to all open files */

	for (j = 0; j < NR_OPEN; j++) {
		if ((filp = current->files.fd[j])) {
			filp->f_count++;
		}
	}
	t->fs.root->i_count++;
	t->fs.pwd->i_count++;
	/* Set up our family tree */
	t->p_parent = current;
	t->p_nextsib = NULL;
	t->p_child = NULL;
	t->child_lastend = 0;
	t->lastend_status = 0;
	if (current->p_child) {
		current->p_child->p_nextsib = t;
		t->p_prevsib = current->p_child;
	}
	else {
		t->p_prevsib = NULL;
	}
	current->p_child = t;

	/*
	 *	Wake our new process
	 */
	 
	wake_up_process(t);
	
	/*
	 *	Return the created task.
	 */

	return t->pid;
}

