/*
 *  	Minix 16bit format binary image loader and exec support routines.
 */

#include <linuxmt/vfs.h>
#include <linuxmt/types.h>
#include <linuxmt/utime.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/stat.h>
#include <linuxmt/string.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/signal.h>
#include <linuxmt/tty.h>
#include <linuxmt/time.h>
#include <linuxmt/mm.h>
#include <linuxmt/minix.h>
#include <linuxmt/msdos.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>

static struct file file;
#ifdef CONFIG_EXEC_MINIX
static struct minix_exec_hdr mh;
#endif
#ifdef CONFIG_EXEC_MSDOS
static struct msdos_exec_hdr mshdr;
#endif

/*
 *	FIXME: Semaphore on entry needed.
 */
 
int sys_execve(filename,sptr,slen)
char *filename;
char *sptr;
int slen;		/* Size of built stack */
{
	unsigned int result, envp, argv;
	int retval,execformat;
	int ds=current->t_regs.ds;
	unsigned short cseg,dseg;
	unsigned long len;
	struct inode *inode;
	register char *ptr;
	unsigned short count;
	int i, nzero, tmp;

	/*
	 *	Open the image
	 */

	printd_exec1("EXEC: opening file %s\n", filename);	
 
	retval = open_namei(filename, 0, 0, &inode, NULL);
	
	printd_exec1("EXEC: open returned %d\n", retval);
	if(retval)
		return retval;
	
	printd_exec("EXEC: start building a file handle\n");
	/*
	 *	Build a reading file handle
	 */	
	 
	file.f_mode=1;
	file.f_flags=0;
	file.f_count=1;
	file.f_inode=inode;
	file.f_pos=0;
#ifdef BLOAT_FS
	file.f_reada=0;
#endif
	file.f_op = inode->i_op->default_file_ops;
	retval=-ENOEXEC;
	if(!file.f_op)
		goto end_readexec;
	if(file.f_op->open)
		if(file.f_op->open(inode,&file))
			goto end_readexec;
	if(!file.f_op->read)
		goto close_readexec;
		
	printd_exec1("EXEC: Opened ok inode dev = 0x%x\n", inode->i_dev);

#ifdef CONFIG_EXEC_MINIX
		
	/*
	 *	Read the header.
	 */
	 
	current->t_regs.ds=get_ds();
	file.f_pos=0; /* FIXME - should call lseek */
	result=file.f_op->read(inode, &file, &mh, sizeof(mh));
	current->t_regs.ds=ds;

	/*
	 *	Sanity check it.
	 */
	
	if( result != sizeof(mh) || 
		(mh.type!=MINIX_COMBID && mh.type!=MINIX_SPLITID) ||
		mh.chmem<1024 || mh.tseg==0)
	{
		printd_exec1("EXEC: bad header, result %d\n",result);
		retval=-ENOEXEC;
		goto close_readexec;
	}
	execformat=EXEC_MINIX;
/* This looks very hackish and it is
 * but bcc can't handle a goto xyz; and a subsequent xyz:
 * -- simon weijgers
 */
#ifdef CONFIG_EXE_MSDOS
	goto blah;
#endif
#endif

#ifdef CONFIG_EXEC_MSDOS
	/* Read header */
	current->t_regs.ds=get_ds();
	file.f_pos=0;
	result=file.f_op->read(inode, &file, &mshdr, sizeof(mshdr));
	current->t_regs.ds=ds;

	if( (result != sizeof(mshdr)) ||
		(mshdr.magic!=MSDOS_MAGIC) )
	{
		printd_exec1("EXEC: bad header, result %d\n",result);
		retval=-ENOEXEC;
		goto close_readexec;
	}
	execformat=EXEC_MSDOS;
/*	goto blah;
 */
#endif
	
blah:
	printd_exec("EXEC: Malloc time\n");
	/*
	 *	Looks good. Get the memory we need
	 */

	cseg = 0;
	for (i = 0 ; i < MAX_TASKS; i++) {
 	  if ((task[i].state != TASK_UNUSED) && (task[i].t_inode == inode)) {
		cseg = mm_realloc(task[i].mm.cseg); 
		break;
	  }	
	}
	
	if (!cseg) cseg=mm_alloc((int)((mh.tseg+15)>>4),0);
	if(cseg==-1)
	{
		retval=-ENOMEM;
		goto close_readexec;
	}
	 
	/*
	 * mh.chmem is "total size" requested by ld. Note that ld used to ask 
	 * for (at least) 64K
	 */
	len=(mh.chmem+15)&~15L;
	if(len>65535L)
	{
		retval=-ENOMEM;
		mm_free(cseg);
		goto close_readexec;
	}

	printd_exec1("Allocating %d bytes for data segment\n", len);
	dseg=mm_alloc((int)(len>>4),0);
	if(dseg==-1)
	{
		retval=-ENOMEM;
		mm_free(cseg);
		goto close_readexec;
	}
	
	printd_exec2("EXEC: Malloc succeeded - cs=%x ds=%x\n", cseg, dseg);
	current->t_regs.ds=cseg;
	result=file.f_op->read(inode, &file, 0, mh.tseg);
	current->t_regs.ds=ds;
	if(result!=mh.tseg)
	{
		printd_exec2("EXEC(tseg read): bad result %d, expected %d\n",result,mh.tseg);
		retval=-ENOEXEC;
		mm_free(cseg);
		mm_free(dseg);
		goto close_readexec;
	}

	current->t_regs.ds=dseg;
	result=file.f_op->read(inode, &file, 0, mh.dseg);
	current->t_regs.ds=ds;
	if(result!=mh.dseg)
	{
		printd_exec2("EXEC(dseg read): bad result %d, expected %d\n",result,mh.dseg);
		retval=-ENOEXEC;
		mm_free(cseg);
		mm_free(dseg);
		goto close_readexec;
	}
	
	/*
	 *	Wipe the BSS.
	 */
	 
	ptr = (char *)mh.dseg;
	count = mh.bseg;
	while (count)
		pokeb(dseg, ptr++, 0), count--;
	/* fmemset should work, but doesn't */
/*	fmemset(ptr, dseg, 0, count); */
	
	/*
	 *	Copy the stack
	 */
	
	ptr = (char *)(len-slen);
	count = slen;
	fmemcpy(dseg, ptr, current->mm.dseg, sptr, count);

	/* argv and envp are two NULL-terminated arrays of pointers, located
	 * right after argc.  This fixes them up so that the loaded program
	 * gets the right strings. */

	nzero = 0; i = ptr + 2;
	while (nzero < 2) {
		if ((tmp = peekw(dseg, i)) != 0) {
			pokew(dseg, i, tmp + ptr);
		} else {
			nzero++;
		}
		i += 2;
	}; 

	/*
	 *	Now flush the old binary out.
	 */
	
	if(current->mm.cseg)
		mm_free(current->mm.cseg);
	if(current->mm.dseg)
		mm_free(current->mm.dseg);
	current->mm.cseg=cseg;
	current->mm.dseg=dseg;	printd_exec("EXEC: old binary flushed.\n");
	
	/*
	 *	FIXME: Clear signal handlers..
	 */
	 
	/*
	 *	Arrange our return to be to CS:0
	 *	(better to use the entry offset in the header)
	 */
	
	current->t_regs.cs=cseg;
	current->t_regs.ds=dseg;
	current->t_regs.ss=dseg;
	current->t_regs.sp=len-slen;	/* Just below the arguments */ 
	current->t_begstack=len-slen;
	current->t_endtext=mh.dseg;	/* Needed for sys_brk() */
	current->t_endbrk=current->t_enddata=mh.dseg+mh.bseg;
	current->t_endstack=len;	/* with 64K = 0000 but that's OK */
	current->t_inode=inode;
	arch_setup_kernel_stack(current);
	
	retval = 0;

	/*
	 *	Done
	 */

close_readexec:
	if(file.f_op->release)
		file.f_op->release(inode,&file);
end_readexec:	 
			   
	/*
	 *	This will return onto the new user stack and to cs:0 of
	 *	the user process.
	 */
	 
	return retval;
}
	
