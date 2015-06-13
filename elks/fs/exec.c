/*
 *  	Minix 16bit format binary image loader and exec support routines.
 *
 *	5th Jan 1999	Alistair Riddoch (ajr@ecs.soton.ac.uk)
 *			Added shared lib support consisting of a sys_dlload
 *			Which loads dll text into new code segment, and dll
 *			data into processes data segment.
 *			This required support for the mh.unused field in the
 *			bin header to contain the size of the dlls data, which
 *			is used in sys_exec to offset loading of the processes
 *			data at run time, new MINIX_DLLID library format, and
 *			new MINIX_S_SPLITID binary format.
 *	21th Jan 2000	Alistair Riddoch (ajr@ecs.soton.ac.uk)
 *			Rethink of binary format leads me to think that the
 *			shared library route is not worth pursuing with the
 *			implemented so far. Removed hacks from exec()
 *			related to this, and instead add support for
 *			binaries which have the stack below the data
 *			segment. Binaries in this form have a large minix
 *			format header (0x30 bytes rather than 0x20) which
 *			contain a field which is reffered to in the a.out.h
 *			file as "data relocation base". This is taken as the
 *			address within the data segment where the program
 *			expects its data segment to begin. Below this we
 *			then put the stack. This requires adding support
 *			for allocating blocks of memory which do not start
 *			at the bottom of the segment. See arch/i86/mm/malloc.c
 *			for details.
 */

#include <linuxmt/vfs.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/stat.h>
#include <linuxmt/string.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/signal.h>
#include <linuxmt/time.h>
#include <linuxmt/mm.h>
#include <linuxmt/minix.h>
#include <linuxmt/dll.h>
#include <linuxmt/msdos.h>
#include <linuxmt/init.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>

static struct minix_exec_hdr mh;
static struct minix_supl_hdr msuph;

#define INIT_HEAP 0x0

int sys_execve(char *filename, char *sptr, size_t slen)
{
    void *ptr;
    struct inode *inode;
    struct file *filp;
    register __registers *tregs;
    unsigned int suidfile, sgidfile;
    int retval;
    __u16 ds;
    seg_t cseg, dseg, stack_top = 0;
    uid_t effuid;
    gid_t effgid;
    lsize_t len;
    size_t result;

    /*
     *      Open the image
     */
    /*debug1("EXEC: opening file: %s\n", filename);*/
    debug1("EXEC: slen = %d\n", slen);

    retval = open_namei(filename, 0, 0, &inode, NULL);

    debug1("EXEC: open returned %d\n", -retval);
    if (retval)
	goto error_exec1;

    debug("EXEC: start building a file handle\n");

    /*
     *      Get a reading file handle
     */
    if((retval = open_filp(O_RDONLY, inode, &filp)))
	goto error_exec2;
    /*
     *      Read the header.
     */
    tregs = &current->t_regs;
    ds = tregs->ds;

    retval = -ENOEXEC;
    if(!(filp->f_op) || !(filp->f_op->read))
	goto error_exec3;

    debug1("EXEC: Inode dev = 0x%x opened OK.\n", inode->i_dev);

    /*
     *      can I trust the following fields?
     */
    {
	register struct inode *pinode = inode;

	suidfile = pinode->i_mode & S_ISUID;
	sgidfile = pinode->i_mode & S_ISGID;
	effuid = pinode->i_uid;
	effgid = pinode->i_gid;

	tregs->ds = kernel_ds;
	result = filp->f_op->read(pinode, filp, &mh, sizeof(mh));
	/*tregs->ds = ds;*/
    }

    /*
     *      Sanity check it.
     */

    if (result != sizeof(mh) ||
	(mh.type != MINIX_SPLITID) || mh.chmem < 1024 || mh.tseg == 0) {
	debug1("EXEC: bad header, result %u\n", result);
	goto error_exec3;
    }

#ifdef CONFIG_EXEC_ELKS
    if ((unsigned int) mh.hlen == 0x30) {
	/* BIG HEADER */
	/*tregs->ds = kernel_ds;*/
	result = filp->f_op->read(inode, filp, &msuph, sizeof(msuph));
	/*tregs->ds = ds;*/
	if (result != sizeof(msuph)) {
	    debug1("EXEC: Bad secondary header, result %u\n", result);
	    goto error_exec3;
	}
	stack_top = msuph.msh_dbase;
	if(stack_top & 0xf){
	     goto error_exec3;
	}
	debug1("EXEC: New type executable stack = %x\n", stack_top);
    }
#else
    if((unsigned int) mh.hlen != 0x20){
        goto error_exec3;
    }
#endif

    debug("EXEC: Malloc time\n");

    /*
     *      Looks good. Get the memory we need
     */

    cseg = 0;
    {
        register struct task_struct *p = &task[0];

	do {
	    if ((p->state != TASK_UNUSED) && (p->t_inode == inode)) {
		cseg = mm_realloc(p->mm.cseg);
		break;
	    }
	} while (++p < &task[MAX_TASKS]);
    }

    retval = -ENOMEM;
    if (!cseg) {
        cseg = mm_alloc((segext_t) ((mh.tseg + 15) >> 4));
        if (!cseg) {
            goto error_exec3;
        }
        tregs->ds = cseg;
        result = filp->f_op->read(inode, filp, 0, mh.tseg);
        /*tregs->ds = ds;*/
        if (result != mh.tseg) {
            debug2("EXEC(tseg read): bad result %u, expected %u\n",
	       result, mh.tseg);
	    retval = -ENOEXEC;
	    goto error_exec4;
        }
    }
    else {
        filp->f_pos += mh.tseg;
    }

    /*
     * mh.chmem is "total size" requested by ld. Note that ld used to ask
     * for (at least) 64K
     */
    if (stack_top) {
	len = mh.dseg + mh.bseg + stack_top + INIT_HEAP;
    } else {
	len = mh.chmem;
    }
    len = (len + 15) & ~15L;
    if (len > (lsize_t) 0x10000L) {
	goto error_exec4;
    }

    debug1("EXEC: Allocating %ld bytes for data segment\n", len);

    dseg = mm_alloc((segext_t) (len >> 4));
    if (!dseg) {
	goto error_exec4;
    }

    debug2("EXEC: Malloc succeeded - cs=%x ds=%x\n", cseg, dseg);

    retval = -ENOEXEC;
    tregs->ds = dseg;
    result = filp->f_op->read(inode, filp, (char *)stack_top, mh.dseg);
    tregs->ds = ds;
    if (result != mh.dseg) {
	debug2("EXEC(dseg read): bad result %d, expected %d\n",
	       result, mh.dseg);
	goto error_exec5;
    }

    /*
     *      Wipe the BSS.
     */
    fmemset((char *)((seg_t)mh.dseg + stack_top), dseg, 0, (__u16) mh.bseg);

    /*
     *      Copy the stack
     */
    ptr = (stack_top)
	? (char *) (stack_top - slen)
	: (char *) ((size_t)len - slen);
    fmemcpy(dseg, (__u16) ptr, current->mm.dseg, (__u16) sptr, (__u16) slen);

    /* argv and envp are two NULL-terminated arrays of pointers, located
     * right after argc.  This fixes them up so that the loaded program
     * gets the right strings. */

    {
	register __u16 *p = (__u16 *) ptr, nzero = 0, tmp;

	while (nzero < 2) {
	    p++;
	    if ((tmp = peekw(dseg, (__u16) p)) != 0) {
		pokew(dseg, (__u16) p, (__u16) (((char *) ptr) + tmp));
	    } else
		nzero++;	/* increments for each array traversed */
	}
    }

    /*
     *      Now flush the old binary out.
     */

    {
	register __ptask currentp = current;
	if (currentp->mm.cseg)
	    mm_free(currentp->mm.cseg);
	if (currentp->mm.dseg)
	    mm_free(currentp->mm.dseg);
	currentp->mm.cseg = cseg;
	currentp->mm.dseg = dseg;
    }
    debug("EXEC: old binary flushed.\n");

    {
	register char *pi;
	/*
	 *      Clear signal handlers..
	 */
	pi = (char *) NSIG;
	do {
	    --pi;
	    current->sig.action[(int)pi].sa_handler = NULL;
	} while (pi);

	/*
	 *      Close required files..
	 */
	pi = (char *) NR_OPEN;
	do {
	    --pi;
	    if (FD_ISSET(((int)pi), &current->files.close_on_exec))
		sys_close((int)pi);
	} while (pi);
    }
    /*
     *      Arrange our return to be to CS:0
     *      (better to use the entry offset in the header)
     */

    tregs->ss = dseg;
    tregs->sp = ((__u16) ptr);		/* Just below the arguments */
    tregs->cs = cseg;
    tregs->ds = dseg;

    {
	register __ptask currentp = current;

	currentp->t_begstack = ((__pptr) ptr);
	currentp->t_endbrk = (__pptr) (mh.dseg + mh.bseg + stack_top);
	currentp->t_enddata = (__pptr) (mh.dseg + stack_top);
	/* Needed for sys_brk() */
	currentp->t_endseg = (__pptr) len;
	currentp->t_inode = inode;
	arch_setup_kernel_stack(currentp);

    /* this could be a good place to set the effective user identifier
     * in case the suid bit of the executable had been set */

	if (suidfile)
	    currentp->euid = effuid;
	if (sgidfile)
	    currentp->egid = effgid;

	wake_up(&currentp->p_parent->child_wait);
    }

    /*
     *      Done
     */

    /*
     *      This will return onto the new user stack and to cs:0 of
     *      the user process.
     */

    retval = 0;
    goto normal_out;

  error_exec5:
    mm_free(dseg);

  error_exec4:
    mm_free(cseg);

  error_exec3:
    tregs->ds = ds;
  normal_out:
    close_filp(inode, filp);

  error_exec2:
    if(retval)
	iput(inode);
  error_exec1:
    debug1("EXEC: Returning %d\n", retval);
    return retval;
}
