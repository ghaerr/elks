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
#include <linuxmt/msdos.h>
#include <linuxmt/init.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>

static struct minix_exec_hdr mh;
#ifdef CONFIG_EXEC_ELKS
static struct minix_supl_hdr msuph;
#endif

// Default data sizes

#define INIT_HEAP  0x0     // For future use (inverted heap and stack)
#define INIT_STACK 0x4000  // 16 K (space for both stack and heap)


int sys_execve(char *filename, char *sptr, size_t slen)
{
    register __ptask currentp;
    struct inode *inode;
    struct file *filp;
    int retval;
    __u16 ds;
    seg_t base_data = 0;
    segment_s * seg_code;
    segment_s * seg_data;
    lsize_t len;

    /* Open the image */
    /*debug1("EXEC: opening file: %s\n", filename);*/
    debug1("EXEC: slen = %d\n", slen);

    retval = open_namei(filename, 0, 0, &inode, NULL);

    debug1("EXEC: open returned %d\n", -retval);
    if (retval) goto error_exec1;

    debug("EXEC: start building a file handle\n");

    /* Get a reading file handle */
    if ((retval = open_filp(O_RDONLY, inode, &filp))) goto error_exec2;

    /* Look for the binary in memory */
    seg_code = 0;
    currentp = &task[0];
    do {
	if ((currentp->state <= TASK_STOPPED) && (currentp->t_inode == inode)) {
	    debug_wait("EXEC found copy\n");
	    seg_code = currentp->mm.seg_code;
	    break;
	}
    } while (++currentp < &task[MAX_TASKS]);

    /* Read the header */
    currentp = current;
    ds = currentp->t_regs.ds;

    if (!(filp->f_op) || !(filp->f_op->read)) goto error_exec2_5;

    debug1("EXEC: Inode dev = 0x%x opened OK.\n", inode->i_dev);

    currentp->t_regs.ds = kernel_ds;
    retval = filp->f_op->read(inode, filp, (char *) &mh, sizeof(mh));

    /* Sanity check it.  */
    if (retval != (int)sizeof(mh) ||
	(mh.type != MINIX_SPLITID_AHISTORICAL && mh.type != MINIX_SPLITID) ||
	mh.tseg == 0) {
	debug1("EXEC: bad header, result %u\n", retval);
	goto error_exec3;
    }

    /*
     * Size for data segment
     * mh.chmem was used by old ld86
     * New LD script sets this to zero (default)
     */
    len = mh.chmem;
    if (!len) len = mh.dseg + mh.bseg + INIT_HEAP + INIT_STACK;

    // TODO: revise the ELKS specific executable format
    // as we now master the executable header content
    // with the new GNU build tool chain (custom LD script)

#ifdef CONFIG_EXEC_ELKS
    if ((unsigned int) mh.hlen == 0x30) {
	/* BIG HEADER */
	retval = filp->f_op->read(inode, filp, (char *) &msuph, sizeof(msuph));
	if (retval != (int)sizeof(msuph)) {
	    debug1("EXEC: Bad secondary header, result %u\n", retval);
	    goto error_exec3;
	}
	base_data = msuph.msh_dbase;
	if (base_data & 0xf) goto error_exec3;
	debug1("EXEC: New type executable stack = %x\n", base_data);
	len = mh.dseg + mh.bseg + base_data + INIT_HEAP;
    }
    else
#endif
    if ((unsigned int) mh.hlen != 0x20) goto error_exec3;

    len = (len + 15) & ~15L;
    if (len > (lsize_t) 0xFFFFL) goto error_exec3;  // 64K - 1 to avoid 16 bits rounding

    debug("EXEC: Malloc time\n");

    /*
     *      Looks good. Get the memory we need
     */
    if (!seg_code) {
	retval = -ENOMEM;
	seg_code = seg_alloc((segext_t) ((mh.tseg + 15) >> 4));
	if (!seg_code) goto error_exec3;
	currentp->t_regs.ds = seg_code->base;  // segment used by read()
	retval = filp->f_op->read(inode, filp, 0, (size_t)mh.tseg);
	if (retval != (int)mh.tseg) {
	    debug2("EXEC(tseg read): bad result %u, expected %u\n",
	    retval, mh.tseg);
	    goto error_exec4;
	}
    } else {
	seg_get (seg_code);
	filp->f_pos += mh.tseg;
    }

    debug1("EXEC: Allocating %lu bytes for data segment\n", len);

    retval = -ENOMEM;
    seg_data = seg_alloc ((segext_t) (len >> 4));
    if (!seg_data) goto error_exec4;

    debug2("EXEC: Malloc succeeded - cs=%x ds=%x\n", seg_code->base, seg_data->base);

    currentp->t_regs.ds = seg_data->base;  // segment used by read()
    retval = filp->f_op->read(inode, filp, (char *)base_data, (size_t)mh.dseg);

    if (retval != (size_t)mh.dseg) {
	debug2("EXEC(dseg read): bad result %d, expected %d\n", retval, mh.dseg);
	goto error_exec5;
    }
    /* From this point, exec() will surely succeed */

    currentp->t_endseg = (__pptr)len;	/* Needed for sys_brk() */

    /* Copy the command line and environment */
    currentp->t_begstack = (base_data	/* Just above the top of stack */
	? (__pptr)base_data
	: currentp->t_endseg) - slen;
    currentp->t_regs.sp = (__u16)(currentp->t_begstack);
    fmemcpyb((byte_t *)currentp->t_begstack, seg_data->base, (byte_t *)sptr, ds, (word_t) slen);

    /* From this point, the old code and data segments are not needed anymore */

    /* Flush the old binary out.  */
    if (currentp->mm.seg_code) seg_put(currentp->mm.seg_code);
    if (currentp->mm.seg_data) seg_put(currentp->mm.seg_data);
    debug("EXEC: old binary flushed.\n");

    currentp->mm.seg_code = seg_code;
    currentp->t_xregs.cs = seg_code->base;

    currentp->mm.seg_data = seg_data;
    currentp->t_regs.es = currentp->t_regs.ss = seg_data->base;

    /* Wipe the BSS */
    fmemsetb((seg_t) mh.dseg + base_data, seg_data->base, 0, (word_t) mh.bseg);
    {
	register char *pi = (char *)0;

	/* argv and envp are two NULL-terminated arrays of pointers, located
	 * right after argc.  This fixes them up so that the loaded program
	 * gets the right strings. */

	slen = 0;	/* Start skiping argc */
	do {
	    pi = (char *)(((__u16 *)pi) + 1);
	    if ((retval = get_ustack(currentp, (int)pi)) != 0) 
		put_ustack(currentp, (int)pi, (currentp->t_begstack + retval));
	    else slen++;	/* increments for each array traversed */
	} while (slen < 2);
	retval = 0;

	/* Clear signal handlers */
	pi = (char *)0;
	do {
	    currentp->sig.action[(int)pi].sa_handler = NULL;
	} while ((int)(++pi) < NSIG);

	/* Close required files */
	pi = (char *)0;
	do {
	    if (FD_ISSET(((int)pi), &currentp->files.close_on_exec))
		sys_close((int)pi);
	} while ((int)(++pi) < NR_OPEN);

    /* this could be a good place to set the effective user identifier
     * in case the suid bit of the executable had been set */

	pi = (char *)inode;
	currentp->t_inode = (struct inode *)pi;

    /* can I trust the following fields?  */
	if (((struct inode *)pi)->i_mode & S_ISUID)
	    currentp->euid = ((struct inode *)pi)->i_uid;
	if (((struct inode *)pi)->i_mode & S_ISGID)
	    currentp->egid = ((struct inode *)pi)->i_gid;
    }

    currentp->t_enddata = (__pptr) ((__u16)mh.dseg + (__u16)mh.bseg + base_data);
    currentp->t_endbrk =  currentp->t_enddata;

    /*
     *      Arrange our return to be to CS:entry
     */
    arch_setup_user_stack(currentp, (word_t) mh.entry);

    wake_up(&currentp->p_parent->child_wait);

    /* Done */

    /*
     *      This will return onto the new user stack and to cs:0 of
     *      the user process.
     */
    goto normal_out;

  error_exec5:
    seg_put (seg_data);

  error_exec4:
    seg_put (seg_code);

  error_exec3:
    currentp->t_regs.ds = ds;
  error_exec2_5:
    if (retval >= 0)
	retval = -ENOEXEC;
  normal_out:
    close_filp(inode, filp);

    if (retval)
  error_exec2:
	iput(inode);
  error_exec1:
    debug1("EXEC: Returning %d\n", retval);
    return retval;
}
