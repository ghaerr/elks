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
 *			shared library route is not worth persuing with the
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
 *			For allocating blocks of memory which do not start
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
#include <linuxmt/debug.h>

#include <arch/segment.h>

static struct file file;
#ifdef CONFIG_EXEC_MINIX
static struct minix_exec_hdr mh;
static struct minix_supl_hdr msuph;
#endif
#ifdef CONFIG_EXEC_MSDOS
static struct msdos_exec_hdr mshdr;
#endif

#define INIT_HEAP 0x1000

/*
 *	FIXME: Semaphore on entry needed.
 */
#ifndef CONFIG_NOFS 
int sys_execve(filename,sptr,slen)
char *filename;
char *sptr;
int slen;		/* Size of built stack */
{
	unsigned int result;
	int retval,execformat;
	int ds=current->t_regs.ds;
	seg_t cseg,dseg;
	lsize_t len;
	struct inode *inode;
	char *ptr;
	size_t count;
	int i;
	seg_t stack_top = 0;
#ifdef CONFIG_EXEC_SUGID
	unsigned int effuid, effgid;
	int suidfile, sgidfile;
#endif
	register struct file * filp = &file;
	__registers * tregs;


	/*
	 *	Open the image
	 */

	printd_exec("EXEC: opening file");	
 
	retval = open_namei(filename, 0, 0, &inode, NULL);
	
	printd_exec1("EXEC: open returned %d", retval);
	if(retval)
		goto end_readexec;
	
	printd_exec("EXEC: start building a file handle");
	/*
	 *	Build a reading file handle
	 */	
	 
	filp->f_mode=1;
	filp->f_flags=0;
	filp->f_count=1;
	filp->f_inode=inode;
	filp->f_pos=0;
#ifdef BLOAT_FS
	filp->f_reada=0;
#endif
	filp->f_op = inode->i_op->default_file_ops;
	retval=-ENOEXEC;
	if(!filp->f_op)
		goto end_readexec;
	if(filp->f_op->open)
		if(filp->f_op->open(inode,&file))
			goto end_readexec;
	if(!filp->f_op->read)
		goto close_readexec;
		
	printd_exec1("EXEC: Opened ok inode dev = 0x%x", inode->i_dev);

#ifdef CONFIG_EXEC_MINIX
		
	/*
	 *	Read the header.
	 */
	tregs = &current->t_regs;
	tregs->ds=get_ds();
	filp->f_pos=0; /* FIXME - should call lseek */
#ifdef CONFIG_EXEC_SUGID
 /* can I trust the following fields?
  */
	suidfile = inode->i_mode & S_ISUID;
	sgidfile = inode->i_mode & S_ISGID;
	effuid = inode->i_uid;
	effgid = inode->i_gid;
#endif

	result=filp->f_op->read(inode, &file, &mh, sizeof(mh));
	tregs->ds=ds;

	/*
	 *	Sanity check it.
	 */
	
	if( result != sizeof(mh) || 
		(mh.type!=MINIX_SPLITID) ||
		mh.chmem<1024 || mh.tseg==0)
	{
		printd_exec1("EXEC: bad header, result %d",result);
		retval=-ENOEXEC;
		goto close_readexec;
	}
	/* I am so far unsure about whether we need this, or the S_SPLITID format */
	if ((unsigned int)mh.hlen == 0x30) {
		/* BIG HEADER */
		tregs->ds=get_ds();
		result=filp->f_op->read(inode, &file, &msuph, sizeof(msuph));
		tregs->ds=ds;
		if (result != sizeof(msuph)) {
			printd_exec1("EXEC: bad secondary header, result %d",
					result);
			retval=-ENOEXEC;
			goto close_readexec;
		}
		stack_top = (seg_t)msuph.msh_dbase;
		printk("SBASE = %x\n", stack_top);
	}
		
	execformat=EXEC_MINIX;
/* This looks very hackish and it is
 * but bcc can't handle a goto xyz; and a subsequent xyz:
 * -- simon weijgers
 */
#ifdef CONFIG_EXEC_MSDOS
	goto blah;
#endif
#endif

#ifdef CONFIG_EXEC_MSDOS
	/* Read header */
	tregs->ds = get_ds();
	filp->f_pos = 0L;
	result = filp->f_op->read(inode, &file, &mshdr, sizeof(mshdr));
	tregs->ds = ds;

	if ((result != sizeof(mshdr)) || (mshdr.magic != MSDOS_MAGIC) )
	{
		printd_exec1("EXEC: bad header, result %d",result);
		retval=-ENOEXEC;
		goto close_readexec;
	}
	execformat=EXEC_MSDOS;
/*	goto blah;
 */
	
blah:
#endif
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
	
	if (!cseg) {
		cseg=mm_alloc((segext_t)((mh.tseg+15)>>4));
	}
	if (!cseg) {
		retval=-ENOMEM;
		goto close_readexec;
	}
	 
	/*
	 * mh.chmem is "total size" requested by ld. Note that ld used to ask 
	 * for (at least) 64K
	 */
	if (stack_top) {
		len = mh.dseg + mh.bseg + stack_top + INIT_HEAP;
		printk("NB [dseg = %x]\n", (segext_t)(len>>4));
	} else {
		len = mh.chmem;
	}
	len=(len+15)&~15L;
	if (len > 0x10000L)
	{
		retval=-ENOMEM;
		mm_free(cseg);
		goto close_readexec;
	}

	printd_exec1("Allocating %d bytes for data segment", len);
	dseg=mm_alloc((segext_t)(len>>4));
	if(!dseg)
	{
		retval=-ENOMEM;
		mm_free(cseg);
		goto close_readexec;
	}
	
	printd_exec2("EXEC: Malloc succeeded - cs=%x ds=%x", cseg, dseg);
	tregs->ds=cseg;
	result=filp->f_op->read(inode, &file, 0, mh.tseg);
	tregs->ds=ds;
	if(result!=mh.tseg)
	{
		printd_exec2("EXEC(tseg read): bad result %d, expected %d",result,mh.tseg);
		retval=-ENOEXEC;
		mm_free(cseg);
		mm_free(dseg);
		goto close_readexec;
	}

	tregs->ds=dseg;
	result=filp->f_op->read(inode, &file, (char *)stack_top, mh.dseg);
	tregs->ds=ds;
	if(result!=mh.dseg)
	{
		printd_exec2("EXEC(dseg read): bad result %d, expected %d",result,mh.dseg);
		retval=-ENOEXEC;
		mm_free(cseg);
		mm_free(dseg);
		goto close_readexec;
	}
	
	/*
	 *	Wipe the BSS.
	 */
	 
	ptr = ((char *)mh.dseg) + stack_top;
	count = mh.bseg;
	while (count) {
		pokeb(dseg, ptr++, 0);
		count--;
	}
	/* fmemset should work, but doesn't */
/*	fmemset(ptr, dseg, 0, count); */
	
	/*
	 *	Copy the stack
	 */
	
	if (stack_top) {
		ptr = (char *)(stack_top - slen);
	} else {
		ptr = (char *)(len-slen);
	}
	count = slen;
	fmemcpy(dseg, ptr, current->mm.dseg, sptr, count);

	/* argv and envp are two NULL-terminated arrays of pointers, located
	 * right after argc.  This fixes them up so that the loaded program
	 * gets the right strings. */

#if 0
	nzero = 0; i = ptr + 2;
	while (nzero < 2) {
		if ((tmp = peekw(dseg, i)) != 0) {
			pokew(dseg, i, tmp + ptr);
		} else {
			nzero++;
		}
		i += 2;
	}; 
#else
	{
	int *p, nzero = 0, tmp;
	p = (int *) ptr;
	while (nzero < 2) {
		p++;
		if ((tmp = peekw(dseg, p)) != 0) {
			pokew(dseg, p, tmp + (int)ptr);
		} else {
			nzero++; /* increments for each array traveresed */
		}
	};
	}
#endif

	/*
	 *	Now flush the old binary out.
	 */
	
	if(current->mm.cseg)
		mm_free(current->mm.cseg);
	if(current->mm.dseg)
		mm_free(current->mm.dseg);
	current->mm.cseg=cseg;
	current->mm.dseg=dseg;
	printd_exec("EXEC: old binary flushed.");
	
	/*
	 *	Clear signal handlers..
	 */
	for (i=0 ; i < NSIG ; i++) {
		current->sig.action[i].sa_handler = NULL;
	}
	/*
	 *	Close required files..
	 */
	for (i=0 ; i<NR_OPEN ; i++) {
		if (FD_ISSET(i,&current->files.close_on_exec)) {
			sys_close(i);
		}
	}

	 
	/*
	 *	Arrange our return to be to CS:0
	 *	(better to use the entry offset in the header)
	 */
	
	tregs->cs=cseg;
	tregs->ds=dseg;
	tregs->ss=dseg;
	tregs->sp=ptr;	/* Just below the arguments */ 
	current->t_begstack=ptr;
#if 0
	current->t_endbrk=current->t_enddata=mh.dseg+mh.bseg;
#endif
	current->t_endbrk=(__pptr)(mh.dseg+mh.bseg+stack_top);
	current->t_enddata=(__pptr)(mh.dseg + stack_top);	/* Needed for sys_brk() */
#if 0
	if (stack_top) {
		current->t_endstack=(__pptr)stack_top;
	} else {
		current->t_endstack=(__pptr)len;	/* with 64K = 0000 but that's OK */
	}
#endif
	current->t_endseg=(__pptr)len;
	current->t_inode=inode;
	arch_setup_kernel_stack(current);

#ifdef CONFIG_EXEC_SUGID
 /* this could be a good place to set the effective user identifier
  * in case the suid bit of the executable had been set */

	if(suidfile) {
		current->euid = effuid;
	}
	if(sgidfile) {
		current->egid = effgid;
	}
#endif


	retval = 0;
	wake_up(&current->p_parent->child_wait);

	/*
	 *	Done
	 */

close_readexec:
	if(filp->f_op->release)
		filp->f_op->release(inode,&file);
end_readexec:	 
			   
	/*
	 *	This will return onto the new user stack and to cs:0 of
	 *	the user process.
	 */
	 
	printd_exec1("EXEC: Returning %d\n", retval);
	return retval;
}
#else
int sys_execve(filename,sptr,slen)
char *filename;
char *sptr;
int slen;		/* Size of built stack */
{
	return -ENOSYS;
}
#endif

#if CONFIG_SHLIB

static struct dll_entry dll[MAX_DLLS];

#if 0 /* Old bad way of loading dlls */
unsigned short sys_dlload(filename)
char * filename;
{
	int retval,i,dno=-1;
	struct inode *inode;
	struct file * filp = &file;
	unsigned short size;
	unsigned short cseg = 0;
	unsigned short ds=current->t_regs.ds;
	unsigned int result;
	__registers * tregs;

	printk("DLLOAD: opening dll file.\n");
	
	retval = open_namei(filename, 0, 0, &inode, NULL);

	printk("DLLOAD: open returned %d\n", retval);
	if (retval)
		return retval;

	printk("DLLOAD: building file handler\n");

	filp->f_mode=1;
	filp->f_flags=0;
	filp->f_count=1;
	filp->f_inode=inode;
	filp->f_pos=0;
	filp->f_op = inode->i_op->default_file_ops;
	retval=-ENOEXEC;
	if(!filp->f_op)
		goto end_readexec;
	if(filp->f_op->open)
		if(filp->f_op->open(inode,&file))
			goto end_readexec;
	if(!filp->f_op->read)
		goto close_readexec;

	if (inode->i_size > 65535L) {
		goto close_readexec;
		printk("DLLOAD: Too big\n");
	}
	size = (unsigned short)inode->i_size;
	printk("DLLOAD: The file is %d bytes.\n",size);


	for (i = 0; i < MAX_DLLS; i++) {
		printk("Found dll no. %d, %s.\n", i, dll[i].d_state ? "used" : "unused");
		if ((dll[i].d_state == DLL_USED) && (dll[i].d_inode == inode)) {			printk("DLLOAD: dll no. %d is the one we want to load, leave it then.\n");
			cseg = dll[i].d_cseg;
			break;
		}
		if (dll[i].d_state != DLL_USED) {
			dno = i;
		}
	}

	if (dno == -1) {
		printk("DLLOAD: No free lib slots.\n");
		retval = -ENOMEM;
		goto close_readexec;
	}

	if (!cseg) {
		printk("DLLOAD: Mallocing some RAM for the dll.\n");
		cseg = mm_alloc((segext_t)((size+15)>>4));
		if (!cseg) {
			printk("DLLOAD: No memory.\n");
			retval=-ENOMEM;
			goto close_readexec;
		}
		tregs = &current->t_regs;
		tregs->ds=cseg;
		result = filp->f_op->read(inode, &file, 0, size);
		tregs->ds=ds;
		if (result!=size) {
			printk("DLLOAD: Bad read expected %d got %d.\n",size, result);
			mm_free(cseg);
			goto close_readexec;
		}
		dll[dno].d_state = DLL_USED;
		dll[dno].d_cseg = cseg;
		dll[dno].d_inode = inode;
	}
	printk("DLLOAD: Returning code seg %d.\n",cseg);
	return cseg;

close_readexec:
	if(filp->f_op->release)
		filp->f_op->release(inode,&file);
end_readexec:

	return retval;
}

#else /* New good way of loading text and data dlls */

unsigned short sys_dlload(filename, dll_cseg)
char * filename;
unsigned short * dll_cseg;
{
	int retval,i,dno=-1;
	struct inode *inode;
	struct file * filp = &file;
	seg_t cseg = 0;
	seg_t ds = current->t_regs.ds;
	unsigned int result;
	__registers * tregs;

	/* The dll to be loaded should be a minix splitid format binary */

	printk("DLLOAD: opening dll file.\n");
	
	/* Open the dll in the usual way */

	retval = open_namei(filename, 0, 0, &inode, NULL);

	printk("DLLOAD: open returned %d\n", retval);
	if (retval)
		return retval;

	printk("DLLOAD: building file handler\n");

	/* Build a file pointer to use when reading */

	filp->f_mode=1;
	filp->f_flags=0;
	filp->f_count=1;
	filp->f_inode=inode;
	filp->f_pos=0;
	filp->f_op = inode->i_op->default_file_ops;
	retval=-ENOEXEC;
	if(!filp->f_op)
		goto end_readexec;
	if(filp->f_op->open)
		if(filp->f_op->open(inode,&file))
			goto end_readexec;
	if(!filp->f_op->read)
		goto close_readexec;

	/* Modify the current ds and load the header of the dll into *
	 * kernel space. */

	tregs = &current->t_regs;
	tregs->ds=get_ds();
	filp->f_pos=0; /* FIXME - should call lseek */
	result=filp->f_op->read(inode, &file, &mh, sizeof(mh));
	tregs->ds=ds;

	/* Check it really is a splitid binary */

	if( result != sizeof(mh) || 
		(mh.type!=MINIX_DLLID) ||
		mh.chmem<1024 || mh.tseg==0)
	{
		printd_exec1("DLLOAD: bad header, result %d",result);
		retval=-ENOEXEC;
		goto close_readexec;
	}

	/* Find out if we have loaded this one before, if not find a slot *
	 * for it. */

	for (i = 0; i < MAX_DLLS; i++) {
		printk("Found dll no. %d, %s.\n", i, dll[i].d_state ? "used" : "unused");
		if ((dll[i].d_state == DLL_USED) && (dll[i].d_inode == inode)) {			printk("DLLOAD: dll no. %d is the one we want to load, leave it then.\n");
			cseg = dll[i].d_cseg;
			break;
		}
		if (dll[i].d_state != DLL_USED) {
			dno = i;
		}
	}

	/* Oops, no more dlls allowed. This should never happen as it is
	 * unlikely that more than 1 dll will exist. */

	if (dno == -1) {
		printk("DLLOAD: No free lib slots.\n");
		retval = -ENOMEM;
		goto close_readexec;
	}

	/* If the dll has not been loaded before, allocate some ram and load *
	 * its text segment. */

	if (!cseg) {
		printk("DLLOAD: Mallocing some RAM for the dll code.\n");
		cseg = mm_alloc((segext_t)((mh.tseg+15)>>4));
		if (!cseg) {
			printk("DLLOAD: No memory.\n");
			retval=-ENOMEM;
			goto close_readexec;
		}
		tregs->ds=cseg;
		result = filp->f_op->read(inode, &file, 0, mh.tseg);
		tregs->ds=ds;
		if (result!=mh.tseg) {
			printk("DLLOAD: Bad read expected %d got %d.\n",mh.tseg, result);
			mm_free(cseg);
			goto close_readexec;
		}
		dll[dno].d_state = DLL_USED;
		dll[dno].d_cseg = cseg;
		dll[dno].d_inode = inode;
	}

	/* Load the dlls data segment into the current processes data segment *
	 * Space should have been assigned for this at compile time, and      *
	 * allocated when the program was loaded */

	filp->f_pos = (off_t)( sizeof(mh) + mh.tseg );
	result = filp->f_op->read(inode, &file, 0, mh.dseg);
	if(result!=mh.dseg) {
			printk("DLLOAD: Bad read expected %d got %d.\n",mh.dseg, result);
	}
		
	printk("DLLOAD: Returning code seg %d.\n",cseg);
	verified_memcpy_tofs(dll_cseg, &cseg, sizeof(unsigned short));
	return 0;

close_readexec:
	if(filp->f_op->release)
		filp->f_op->release(inode,&file);
end_readexec:

	return retval;
}

#endif /* 0 */
#endif /* CONFIG_SHLIB */
