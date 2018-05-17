/*
 * This file contains the procedures for the handling of select
 *
 * Created for Linux based loosely upon Mathius Lattner's minix
 * patches by Peter MacDonald. Heavily edited by Linus.
 *
 *  4 February 1994
 *     COFF/ELF binary emulation. If the process has the STICKY_TIMEOUTS
 *     flag set in its personality we do *not* modify the given timeout
 *     parameter to reflect time remaining.
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/signal.h>
#include <linuxmt/stat.h>
#include <linuxmt/string.h>
#include <linuxmt/time.h>
#include <linuxmt/types.h>

#if 0
#include <linuxmt/personality.h>
#endif

#include <arch/segment.h>
#include <arch/system.h>

#define ROUND_UP(x,y) (((x)+(y)-1)/(y))

/*
 * Ok, Peter made a complicated, but straightforward multiple_wait() function.
 * I have rewritten this, taking some shortcuts: This code may not be easy to
 * follow, but it should be free of race-conditions, and it's practical. If
 * you understand what I'm doing here, then you understand how the linux
 * sleep/wakeup mechanism works.
 *
 * Two very simple procedures, select_wait() and free_wait() make all the work.
 * select_wait() is a inline-function defined in <linux/sched.h>, as all select
 * functions have to call it to add an entry to the select table.
 */

/*
 * I rewrote this again to make the select_table size variable, take some
 * more shortcuts, improve responsiveness, and remove another race that
 * Linus noticed.  -- jrs
 */

struct wait_queue select_queue;  /* magic queue - see sleepwake.c */

/* Add queue to polled ones */

void select_wait (struct wait_queue *q)
{
	int n;
	struct wait_queue **p;

	for (n = 0; n < POLL_MAX; n++) {
		p = &(current->poll [n]);
		if (!*p) {
			*p = q;
			return;
		}
	}

	panic ("select_wait:no slot left");
}

/* Return true if queue is polled */

int select_poll (struct task_struct * t, struct wait_queue *q)
{
	int n;
	struct wait_queue *p;

	for (n = 0; n < POLL_MAX; n++) {
		p = t->poll [n];
		if (!p) return 0;
		if (p == q) return 1;
	}

	panic ("select_poll:no slot found\n");
	return 0;
}

/*
 * The check function checks the ready status of a file using the vfs layer.
 *
 * If the file was not ready we were added to its wait queue.  But in
 * case it became ready just after the check and just before it called
 * select_wait, we call it again, knowing we are already on its
 * wait queue this time.  The second call is not necessary if the
 * select_table is NULL indicating an earlier file check was ready
 * and we aren't going to sleep on the select_table.  -- jrs
 */

static int check(int flag, register struct file *file)
{
    register struct file_operations *fops;

    if ((fops = file->f_op) && fops->select)
	return (fops->select(file->f_inode, file, flag));

    return (flag != SEL_EX);
}

static int do_select(int n, fd_set * in, fd_set * out, fd_set * ex,
		     fd_set * res_in, fd_set * res_out, fd_set * res_ex)
{
    fd_set set;
    int count = -1;
    register char *pi;
    register struct file **filp;

    set = *in | *out | *ex;
    filp = current->files.fd;
    for (pi = 0; set && ((int)pi < n); pi++, set >>= 1) {
	if ((int)set & 1) {
	    if ((*filp == NULL) || ((*filp)->f_inode == NULL)) return -EBADF;
	    count = (int)pi;
	}
	filp++;
    }
    n = count + 1;
    count = 0;
    wait_set(&select_queue);
    current->state = TASK_INTERRUPTIBLE;
  repeat:
    memset (current->poll, 0, sizeof (struct wait_queue *) * POLL_MAX);
    filp = current->files.fd;
    for (pi = 0; ((int)pi) < n; pi++, filp++) {
	if (*filp) {
	    if (FD_ISSET(((int)pi), in) && check(SEL_IN, *filp)) {
		FD_SET(((int)pi), res_in);
		count++;
	    }
	    if (FD_ISSET(((int)pi), out) && check(SEL_OUT, *filp)) {
		FD_SET(((int)pi), res_out);
		count++;
	    }
	    if (FD_ISSET(((int)pi), ex) && check(SEL_EX, *filp)) {
		FD_SET(((int)pi), res_ex);
		count++;
	    }
	}
    }
    if (!count && current->timeout
	&& !(current->signal /* & ~currentp->blocked */ )) {
	schedule();
	goto repeat;
    }

    memset (current->poll, 0, sizeof (struct wait_queue *) * POLL_MAX);
    current->state = TASK_RUNNING;
    wait_clear(&select_queue);
    return count;
}

/*
 * We do a VERIFY_WRITE here even though we are only reading this time:
 * we'll write to it eventually..
 *
 */
static int get_fd_set(register fd_set * fs_pointer, register fd_set * fdset)
{
    if (fs_pointer)
	return verified_memcpy_fromfs((char *) fdset, fs_pointer,
				      sizeof(fd_set));

    memset(fdset, 0, sizeof(fd_set));
    return 0;
}

static void set_fd_set(register fd_set * fs_pointer, fd_set * fdset)
{
    if (fs_pointer) memcpy_tofs(fs_pointer, fdset, sizeof(fd_set));
}

static void zero_fd_set(fd_set * fdset)
{
    memset(fdset, 0, sizeof(fd_set));
}


int sys_select(int n, fd_set * inp, fd_set * outp, fd_set * exp,
	       register struct timeval *tvp)
{
    int error;
    fd_set res_in, in;
    fd_set res_out, out;
    fd_set res_ex, ex;
    jiff_t timeout;

    if (n > NR_OPEN) n = NR_OPEN;
    error = -EINVAL;
    if ((n < 0) || (error = get_fd_set(inp, &in)) ||
	(error = get_fd_set(outp, &out)) || (error = get_fd_set(exp, &ex)))
	goto outl;
    timeout = ~0UL;
    if (tvp) {
	error = verify_area(VERIFY_WRITE, tvp, sizeof(*tvp));
	if (error) goto outl;

	timeout = ROUND_UP(get_user_long(&tvp->tv_usec), (1000000 / HZ));
	timeout += get_user_long(&tvp->tv_sec) * (jiff_t) HZ;
	if (timeout) timeout += jiffies + 1UL;
    }
    zero_fd_set(&res_in);
    zero_fd_set(&res_out);
    zero_fd_set(&res_ex);
    current->timeout = timeout;
    error = do_select(n,
		      /*(fd_set *) */ &in,
		      /*(fd_set *) */ &out,
		      /*(fd_set *) */ &ex,
		      /*(fd_set *) */ &res_in,
		      /*(fd_set *) */ &res_out,
		      /*(fd_set *) */ &res_ex);

    current->timeout = 0UL;
    if (!error && (current->signal /* & ~current->blocked */ ))
	error = -EINTR;
	else {
		/* update arrays even after timeout - see issue #213 */
		set_fd_set(inp, &res_in);
		set_fd_set(outp, &res_out);
		set_fd_set(exp, &res_ex);
	}

  outl:
    return error;
}
