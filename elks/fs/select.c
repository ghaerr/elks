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

struct wait_queue select_poll;	/* magic queue - see sleepwake.c */

/* FIXME *//* should be an inline function */
void select_wait(struct wait_queue *q)
{
    current->pollhash |= 1 << ((((int) q) >> 8) & 15);
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

    if (flag != SEL_EX)
	return 1;

    return 0;
}

static int do_select(int n, fd_set * in, fd_set * out, fd_set * ex,
		     fd_set * res_in, fd_set * res_out, fd_set * res_ex)
{
    int count;
    register __ptask currentp = current;
    fd_set set;
    int max = -1;
    register char *pi;
/*
    int j;

    j = 0;
    for (;;) {
	pi = (char *)(j * __NFDBITS);
	if (((int)pi) >= n)
	    break;
	set = *in | *out | *ex;

	j++;
	for (; set; pi++, set >>= 1) {
	    if (((int)pi) >= n)
		goto end_check;
	    if (!(set & 1))
		continue;
	    if (!currentp->files.fd[(int)pi])
		return -EBADF;
	    if (!currentp->files.fd[(int)pi]->f_inode)
		return -EBADF;
	    max = (int)pi;
	}
    }
  end_check:
*/

    set = *in | *out | *ex;
    for(pi = 0; set && ((int)pi < n); pi++, set >>= 1) {
	if(!(set & 1))
	    continue;
	if (!currentp->files.fd[(int)pi])
	    return -EBADF;
	if (!currentp->files.fd[(int)pi]->f_inode)
	    return -EBADF;
	max = (int)pi;
    }
    n = max + 1;
    count = 0;
  repeat:
    currentp->state = TASK_INTERRUPTIBLE;
    currentp->pollhash = 0;
    wait_set(&select_poll);
    for (pi = 0; ((int)pi) < n; pi++) {
	struct file *file = currentp->files.fd[(int)pi];
	if (file) {
	    if (FD_ISSET(((int)pi), in) && check(SEL_IN, file)) {
		FD_SET(((int)pi), res_in);
		count++;
	    }
	    if (FD_ISSET(((int)pi), out) && check(SEL_OUT, file)) {
		FD_SET(((int)pi), res_out);
		count++;
	    }
	    if (FD_ISSET(((int)pi), ex) && check(SEL_EX, file)) {
		FD_SET(((int)pi), res_ex);
		count++;
	    }
	}
    }
    if (!count && currentp->timeout
	&& !(currentp->signal /* & ~currentp->blocked */ )) {
	schedule();
	wait_clear(&select_poll);
	goto repeat;
    }
    currentp->pollhash = 0;
    wait_clear(&select_poll);
    currentp->state = TASK_RUNNING;
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
    if (fs_pointer)
	memcpy_tofs(fs_pointer, fdset, sizeof(fd_set));
}

static void zero_fd_set(fd_set * fdset)
{
    memset(fdset, 0, sizeof(fd_set));
}

/*
 * We can actually return ERESTARTSYS instead of EINTR, but I'd like to be
 * certain this leads to no problems. So I return EINTR just for safety.
 *
 * Update: ERESTARTSYS breaks at least the xview clock binary, so
 * I'm trying ERESTARTNOHAND which restart only when you want to.
 */

int sys_select(int n, fd_set * inp, fd_set * outp, fd_set * exp,
	       register struct timeval *tvp)
{
    int error;
    fd_set res_in, in;
    fd_set res_out, out;
    fd_set res_ex, ex;
    jiff_t timeout;

    error = -EINVAL;
    if (n < 0)
	goto out;
    if (n > NR_OPEN)
	n = NR_OPEN;
    if ((error = get_fd_set(inp, &in)) ||
	(error = get_fd_set(outp, &out)) || (error = get_fd_set(exp, &ex)))
	goto out;
    timeout = ~0UL;
    if (tvp) {
	error = verify_area(VERIFY_WRITE, tvp, sizeof(*tvp));
	if (error)
	    goto out;

	timeout = ROUND_UP(get_user_long(&tvp->tv_usec), (1000000 / HZ));
	timeout += get_user_long(&tvp->tv_sec) * (jiff_t) HZ;
	if (timeout)
	    timeout += jiffies + 1UL;
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
    if (error < 0)
	goto out;
    if (!error) {
	error = -ERESTARTNOHAND;
	if (current->signal /* & ~current->blocked */ )
	    goto out;
	error = 0;
    }
    set_fd_set(inp, &res_in);
    set_fd_set(outp, &res_out);
    set_fd_set(exp, &res_ex);

  out:
    return error;
}
