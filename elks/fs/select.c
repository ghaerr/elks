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
#include <linuxmt/types.h>
#include <linuxmt/time.h>
#include <linuxmt/fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>
#include <linuxmt/signal.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#if 0
#include <linuxmt/personality.h>
#endif

#include <arch/segment.h>
#include <arch/system.h>

#define ROUND_UP(x,y) (((x)+(y)-1)/(y))

/*
 * Ok, Peter made a complicated, but straightforward multiple_wait() function.
 * I have rewritten this, taking some shortcuts: This code may not be easy to
 * follow, but it should be free of race-conditions, and it's practical. If you
 * understand what I'm doing here, then you understand how the linux
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

static void free_wait(p)
register select_table * p;
{
	register struct select_table_entry * entry = p->entry + p->nr;

	while (p->nr > 0) {
		p->nr--;
		entry--;
		remove_wait_queue(entry->wait_address,&entry->wait);
	}
}

/* FIXME */ /* should be an inline function */
void select_wait(wait_address, p) 
struct wait_queue ** wait_address;
register select_table * p;
{
	register struct select_table_entry * entry;
	if ((p && wait_address) && (p->nr < __MAX_SELECT_TABLE_ENTRIES)) {
		entry = p->entry + p->nr;
		entry->wait_address = wait_address;
		entry->wait.task = current;
		entry->wait.next = NULL;
		add_wait_queue(wait_address,&entry->wait);
		p->nr++;
	}
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

static int check(flag, wait, file)
int flag;
select_table * wait;
struct file * file;
{
	register struct inode * inode;
	register struct file_operations *fops;
	int (*select) ();

	inode = file->f_inode;
	if ((fops = file->f_op) && (select = fops->select)) {
		return (select(inode, file, flag, wait)
		    || (wait && select(inode, file, flag, NULL)));
	}
	if (flag != SEL_EX) {
		return 1;
	}
	return 0;
}

static int do_select(n, in, out, ex, res_in, res_out, res_ex)
int n;
fd_set *in;
fd_set *out;
fd_set *ex;
fd_set *res_in;
fd_set *res_out;
fd_set *res_ex;
{
	int count;
	select_table wait_table, *wait;
	__ptask currentp = current;
	struct select_table_entry entry[__MAX_SELECT_TABLE_ENTRIES];
	fd_set set;
	int i,j;
	int max = -1;

	j = 0;
	for (;;) {
		i = j * __NFDBITS;
		if (i >= n)
			break;
		set = *in | *out | *ex;

		j++;
		for ( ; set ; i++,set >>= 1) {
			if (i >= n)
				goto end_check;
			if (!(set & 1))
				continue;
			if (!currentp->files.fd[i])
				return -EBADF;
			if (!currentp->files.fd[i]->f_inode)
				return -EBADF;
			max = i;
		}
	}
end_check:
	n = max + 1;
	count = 0;
	wait_table.nr = 0;
	wait_table.entry = &entry[0];
	wait = &wait_table;
repeat:
	currentp->state = TASK_INTERRUPTIBLE;
	for (i = 0 ; i < n ; i++) {
		struct file * file = currentp->files.fd[i];
		if (file) {
			if (FD_ISSET(i,in) && check(SEL_IN,wait,file)) {
				FD_SET(i, res_in);
				count++;
				wait = NULL;
			}
			if (FD_ISSET(i,out) && check(SEL_OUT,wait,file)) {
				FD_SET(i, res_out);
				count++;
				wait = NULL;
			}
			if (FD_ISSET(i,ex) && check(SEL_EX,wait,file)) {
				FD_SET(i, res_ex);
				count++;
				wait = NULL;
			}
		}
	}
	wait = NULL;
	if (!count && currentp->timeout && !(currentp->signal/* & ~currentp->blocked*/)) {
		schedule();
		goto repeat;
	}
	free_wait(&wait_table);
	currentp->state = TASK_RUNNING;
	return count;
}

/*
 * We do a VERIFY_WRITE here even though we are only reading this time:
 * we'll write to it eventually..
 *
 */
static int get_fd_set(fs_pointer, fdset)
fd_set * fs_pointer;
fd_set * fdset;
{
	if (fs_pointer) {
		return verified_memcpy_fromfs((char *)fdset, fs_pointer, sizeof(fd_set));
	}
	memset(fdset, 0, sizeof(fd_set));
	return 0;
}

static void set_fd_set(fs_pointer, fdset)
fd_set * fs_pointer;
fd_set * fdset;
{
	if (fs_pointer) {
		memcpy_tofs(fs_pointer, fdset, sizeof(fd_set));
	}
}

static void zero_fd_set(fdset)
register fd_set * fdset;
{
	memset(fdset, 0, sizeof(fd_set));
}		

/*
 * We can actually return ERESTARTSYS instead of EINTR, but I'd
 * like to be certain this leads to no problems. So I return
 * EINTR just for safety.
 *
 * Update: ERESTARTSYS breaks at least the xview clock binary, so
 * I'm trying ERESTARTNOHAND which restart only when you want to.
 */

int sys_select(n, inp, outp, exp, tvp)
int n;
fd_set * inp;
fd_set * outp;
fd_set * exp;
register struct timeval * tvp;
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
	    (error = get_fd_set(outp, &out)) ||
	    (error = get_fd_set(exp, &ex))) goto out;
	timeout = ~0UL;
	if (tvp) {
		error = verify_area(VERIFY_WRITE, tvp, sizeof(*tvp));
		if (error)
			goto out;

		timeout = ROUND_UP(get_fs_long(&tvp->tv_usec),(1000000/HZ));
		timeout += get_fs_long(&tvp->tv_sec) * (jiff_t) HZ;
		if (timeout)
			timeout += jiffies + 1UL;
	}
	zero_fd_set(&res_in);
	zero_fd_set(&res_out);
	zero_fd_set(&res_ex);
	current->timeout = timeout;
	error = do_select(n,
		/*(fd_set *)*/ &in,
		/*(fd_set *)*/ &out,
		/*(fd_set *)*/ &ex,
		/*(fd_set *)*/ &res_in,
		/*(fd_set *)*/ &res_out,
		/*(fd_set *)*/ &res_ex);
#if 0
	timeout = current->timeout - jiffies - 1;
	current->timeout = 0L;
	/* User doesn't really need timeout info back */
	if (timeout < 0L)
		timeout = 0L;
	if (tvp /*&& !(current->personality & STICKY_TIMEOUTS)*/) {
		put_user(timeout/HZ, &tvp->tv_sec);
		timeout %= HZ;
		timeout *= (1000000/HZ);
		put_user(timeout, &tvp->tv_usec);
	}
#else
	current->timeout = 0L;
#endif
	if (error < 0)
		goto out;
	if (!error) {
		error = -ERESTARTNOHAND;
		if (current->signal/* & ~current->blocked*/)
			goto out;
		error = 0;
	}
	set_fd_set(inp, &res_in);
	set_fd_set(outp, &res_out);
	set_fd_set(exp, &res_ex);
out:
	return error;
}



