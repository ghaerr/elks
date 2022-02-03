/*
 *	ELKSEMU	An emulator for Linux8086 binaries.
 *
 *	The 8086 mode code runs in protected mode by way of 16-bit segment
 *	descriptors which we set up in the LDT.
 *	We trap up to 386 mode for system call emulation and naughties. 
 */

#define _GNU_SOURCE  /* for clone */ 
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#include <sched.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <asm/ldt.h>
#include <asm/ptrace-abi.h>
#include "elks.h" 

volatile struct elks_cpu_s elks_cpu;
/* Paragraph aligned */
unsigned char *elks_base, *elks_fartext_base, *elks_data_base;
unsigned short brk_at = 0;

#ifdef DEBUG
#define dbprintf(x) db_printf x
#else
#define dbprintf(x) 
#endif

static int modify_ldt(int func, void *ptr, unsigned long bytes)
{
	return syscall(SYS_modify_ldt, func, ptr, bytes);
}

static void elks_init()
{
	uint64_t ldt[8192];
	struct user_desc cs_desc, fcs_desc, ds_desc;
	memset(ldt, 0, sizeof ldt);
	int cs_idx = 0, fcs_idx, ds_idx,
	    ldt_count = modify_ldt(0, ldt, sizeof ldt);
	unsigned cs, fcs, ds;
	if (ldt_count < 0)
		ldt_count = 0;
	while (cs_idx < ldt_count && ldt[cs_idx] != 0)
		++cs_idx;
	fcs_idx = cs_idx + 1;
	while (fcs_idx < ldt_count && ldt[fcs_idx] != 0)
		++fcs_idx;
	ds_idx = fcs_idx + 1;
	while (ds_idx < ldt_count && ldt[ds_idx] != 0)
		++ds_idx;
	if (cs_idx >= 8192 || fcs_idx >= 8192 || ds_idx >= 8192)
	{
		fprintf(stderr, "No free LDT descriptors for text and data "
				"segments\n");
		exit(255);
	}
	elks_cpu.regs.xcs = cs = cs_idx * 8 + 7;
	/* Stash the far text descriptor number in .orig_eax or .orig_rax
	   first... */
	elks_cpu.regs.orig_xax = fcs = fcs_idx * 8 + 7;
	elks_cpu.regs.xds = elks_cpu.regs.xes = elks_cpu.regs.xss
			  = ds = ds_idx * 8 + 7;
	dbprintf(("LDT descriptor for text is %#x\n", cs));
	dbprintf(("LDT descriptor for data is %#x\n", ds));
	/* Try to place dummy descriptors in the LDT.  For each dummy
	 * descriptor, make sure that either the base or the limit is
	 * non-zero, so that Linux does not think that we are trying to clear
	 * the descriptor entry. */
	memset(&cs_desc, 0, sizeof cs_desc);
	cs_desc.entry_number = cs_idx;
	cs_desc.base_addr = 0x1000;
	cs_desc.limit = 0xfff;
	cs_desc.contents = MODIFY_LDT_CONTENTS_CODE;
	cs_desc.useable = 1;
	memset(&fcs_desc, 0, sizeof fcs_desc);
	fcs_desc.entry_number = fcs_idx;
	fcs_desc.base_addr = 0x1000;
	fcs_desc.limit = 0xfff;
	fcs_desc.contents = MODIFY_LDT_CONTENTS_CODE;
	fcs_desc.useable = 1;
	memset(&ds_desc, 0, sizeof ds_desc);
	ds_desc.entry_number = ds_idx;
	ds_desc.base_addr = 0x1000;
	ds_desc.limit = 0xfff;
	ds_desc.contents = MODIFY_LDT_CONTENTS_DATA;
	ds_desc.useable = 1;
	if (modify_ldt(1, &cs_desc, sizeof cs_desc) != 0
	    || modify_ldt(1, &fcs_desc, sizeof fcs_desc) != 0
	    || modify_ldt(1, &ds_desc, sizeof ds_desc) != 0)
	{
		fprintf(stderr, "Cannot allocate LDT descriptors for text "
				"and data segments\n");
		exit(255);
	}
}

static void elks_take_interrupt(int arg)
{
#if 1
	if(arg==0x20) { minix_syscall(); return; }
#endif
	if(arg!=0x80)
	{
		dbprintf(("Took an int %d\n", arg));
		fflush(stderr);
		kill(getpid(), SIGILL);
		return;
	}
	
	dbprintf(("syscall AX=%x BX=%x CX=%x DX=%x SP=%x "
		  "stack=%x %x %x %x %x\n",
		(unsigned short)elks_cpu.regs.xax,
		(unsigned short)elks_cpu.regs.xbx,
		(unsigned short)elks_cpu.regs.xcx,
		(unsigned short)elks_cpu.regs.xdx,
		(unsigned short)elks_cpu.regs.xsp,
		ELKS_PEEK(unsigned short, elks_cpu.regs.xsp),
		ELKS_PEEK(unsigned short, elks_cpu.regs.xsp + 2),
		ELKS_PEEK(unsigned short, elks_cpu.regs.xsp + 4),
		ELKS_PEEK(unsigned short, elks_cpu.regs.xsp + 6),
		ELKS_PEEK(unsigned short, elks_cpu.regs.xsp + 8)));
		
	elks_cpu.regs.xax = elks_syscall();
	dbprintf(("elks syscall returned %d\n",
		  (int)(short)elks_cpu.regs.xax));
	/* Finally resume the child process */
}

size_t count_argv_envp_bytes(char ** argv, char ** envp)
{
	char **p;
	size_t argv_len=0, argv_count=0;
	size_t envp_len=0, envp_count=0;
	size_t argv_envp_bytes;

	/* How much space for argv */
	for(p=argv; *p; p++)
	{
	   argv_count++; argv_len += strlen(*p)+1;
	}
	if (argv_len % 2)
		argv_len++;

	/* How much space for envp */
	for(p=envp; *p; p++)
	{
	   envp_count++; envp_len += strlen(*p)+1;
	}
	if (envp_len % 2)
		envp_len++;

	/* tot it all up */
	argv_envp_bytes = 2			/* argc */
			  + argv_count * 2 + 2	/* argv */
			  + argv_len
			  + envp_count * 2 + 2	/* envp */
			  + envp_len;

#ifdef DEBUG
	fprintf(stderr, "Argv = (%d,%d), Envp=(%d,%d), stack=%d\n",
		argv_count, argv_len, envp_count, envp_len, argv_envp_bytes);
#endif

	return argv_envp_bytes;
}

static int relocate(unsigned char *place_base, size_t rsize, unsigned cs,
		    unsigned fcs, unsigned ds, int fd)
{
#ifdef DEBUG
	fprintf(stderr, "Applying %#zx bytes of relocations to segment with "
			"linear base %p\n", rsize, place_base);
#endif
	while (rsize >= sizeof(struct minix_reloc)) {
		struct minix_reloc reloc;
		uint16_t val;
		ssize_t r = read(fd, &reloc, sizeof reloc);
		if (r != sizeof(reloc))
			return r >= 0 ? -EINVAL : -errno;
		switch (reloc.r_type) {
		case R_SEGWORD:
			switch (reloc.r_symndx) {
			case S_TEXT:
				val = cs;	break;
			case S_FTEXT:
				val = fcs;	break;
			case S_DATA:
				val = ds;	break;
			default:
				return -EINVAL;
			}
			place_base[(uint16_t)reloc.r_vaddr] =
			    (unsigned char)val;
			place_base[(uint16_t)(reloc.r_vaddr + 1)] =
			    (unsigned char)(val >> 8);
			break;
		default:
			return -EINVAL;
		}
		rsize -= sizeof(struct minix_reloc);
	}
	return 0;
}

static int load_elks(int fd, uint16_t argv_envp_bytes)
{
	/* Load the elks binary image and set it up in the text and data
	   segments. Load CS and DS/SS according to image type. Allocate
	   data segment according to a.out total field */
	struct minix_exec_hdr mh;
	struct elks_supl_hdr esuph;
	struct user_desc cs_desc, fcs_desc, ds_desc;
	uint16_t len, min_len, heap, stack;
	unsigned cs, fcs, ds;
	int retval;

	if(read(fd, &mh,sizeof(mh))!=sizeof(mh))
		return -ENOEXEC;
	if(mh.type!=ELKS_SPLITID && mh.type!=ELKS_SPLITID_AHISTORICAL)
		return -ENOEXEC;

	memset(&esuph, 0, sizeof esuph);

	switch (mh.hlen) {
	default:
		return -ENOEXEC;
	case EXEC_MINIX_HDR_SIZE:
		break;
	case EXEC_RELOC_HDR_SIZE:
	case EXEC_FARTEXT_HDR_SIZE:
		retval = read(fd, &esuph, mh.hlen - sizeof(mh));
		if (retval != mh.hlen - sizeof(mh))
			return retval >= 0 ? -ENOEXEC : -errno;
		if (esuph.msh_tbase != 0 ||
		    esuph.msh_trsize % sizeof(struct minix_reloc) != 0 ||
		    esuph.msh_drsize % sizeof(struct minix_reloc) != 0 ||
		    esuph.esh_ftrsize % sizeof(struct minix_reloc) != 0)
			return -EINVAL;
		/* not supported by elksemu for now */
		if (esuph.msh_dbase != 0)
			return -EINVAL;
		break;
	}

	if (mh.tseg == 0)
		return -ENOEXEC;

#ifdef DEBUG
	fprintf(stderr,"Linux-86 binary - %lX. tseg=%ld dseg=%ld bss=%ld\n",
		mh.type,mh.tseg,mh.dseg,mh.bseg);
#endif

	min_len = mh.dseg;
	if (__builtin_add_overflow(min_len, mh.bseg, &min_len))
		return -EINVAL;
	/*
	 * mh.version == 1: chmem is size of heap, 0 means use default heap
	 * mh.version == 0: old ld86 used chmem as size of data+bss+heap+stack
	 */
	switch (mh.version) {
	default:
		return -ENOEXEC;
	case 1:
		len = min_len;
		stack = mh.minstack ? mh.minstack : INIT_STACK;
		if (__builtin_add_overflow(len, stack, &len))
			return -EFBIG;
		if (__builtin_add_overflow(len, argv_envp_bytes, &len))
			return -E2BIG;
		heap = mh.chmem ? mh.chmem : INIT_HEAP;
		if (heap >= 0xfff0) {
			if (len < 0xfff0)
				len = 0xfff0;
		} else if (__builtin_add_overflow(len, heap, &len))
			return -EFBIG;
		break;

	case 0:
		stack = INIT_STACK;
		len = mh.chmem;
		if (len) {
			if (len <= min_len)
				return -EINVAL;
			heap = len - min_len;
			if (heap < INIT_STACK)
				return -EINVAL;
			heap -= INIT_STACK;
			if (heap < argv_envp_bytes)
				return -E2BIG;
		} else {
			len = min_len;
			if (__builtin_add_overflow(len, INIT_HEAP + INIT_STACK,
						   &len))
				return -EFBIG;
			if (__builtin_add_overflow(len, argv_envp_bytes, &len))
				return -E2BIG;
		}
	}

	if(read(fd,elks_base,mh.tseg)!=mh.tseg)
		return -ENOEXEC;
	elks_fartext_base=elks_base+0x10000;
	if(read(fd,elks_fartext_base,esuph.esh_ftseg)!=esuph.esh_ftseg)
		return -ENOEXEC;
	elks_data_base=elks_base+0x20000;
	if(read(fd,elks_data_base,mh.dseg)!=mh.dseg)
		return -ENOEXEC;
	memset(elks_data_base+mh.dseg,0, mh.bseg);

	cs = elks_cpu.regs.xcs;
	fcs = elks_cpu.regs.orig_xax;
	ds = elks_cpu.regs.xds;

	/*
	 *	Apply relocations
	 */
	retval = relocate(elks_base, esuph.msh_trsize, cs, fcs, ds, fd);
	if (retval != 0)
		return retval;
	retval = relocate(elks_fartext_base, esuph.esh_ftrsize, cs, fcs, ds,
			  fd);
	if (retval != 0)
		return retval;
	retval = relocate(elks_data_base, esuph.msh_drsize, cs, fcs, ds, fd);
	if (retval != 0)
		return retval;

	/*
	 *	Really set up the LDT descriptors
	 */
	memset(&cs_desc, 0, sizeof cs_desc);
	memset(&fcs_desc, 0, sizeof ds_desc);
	memset(&ds_desc, 0, sizeof ds_desc);
	cs_desc.entry_number = cs / 8;
	cs_desc.base_addr = (uintptr_t)elks_base;
	cs_desc.limit = mh.tseg - 1;
	cs_desc.contents = MODIFY_LDT_CONTENTS_CODE;
	cs_desc.seg_not_present = mh.tseg == 0;
	fcs_desc.entry_number = fcs / 8;
	fcs_desc.base_addr = (uintptr_t)elks_fartext_base;
	fcs_desc.limit = esuph.esh_ftseg - 1;
	fcs_desc.contents = MODIFY_LDT_CONTENTS_CODE;
	fcs_desc.seg_not_present = esuph.esh_ftseg == 0;
	ds_desc.entry_number = ds / 8;
	ds_desc.base_addr = (uintptr_t)elks_data_base;
	ds_desc.limit = len - 1;
	ds_desc.contents = MODIFY_LDT_CONTENTS_DATA;
	ds_desc.seg_not_present = len == 0;
	if (modify_ldt(1, &cs_desc, sizeof cs_desc) != 0
	    || modify_ldt(1, &fcs_desc, sizeof fcs_desc) != 0
	    || modify_ldt(1, &ds_desc, sizeof ds_desc) != 0)
	{
		fprintf(stderr, "Cannot set LDT descriptors for text and data "
				"segments\n");
		exit(255);
	}

	elks_cpu.regs.xsp = len;	/* Args stacked later */
	elks_cpu.regs.xip = mh.entry;	/* Run from entry point */
	elks_cpu.child = 0;
	brk_at = mh.dseg + mh.bseg;
	return 0;
}

static int child_idle(void *dummy)
{
	for (;;)
		raise(SIGSTOP);
	return 0;
}

static int wait_for_child(void)
{
	pid_t child = elks_cpu.child, pid;
	int status;
	while ((pid = waitpid(child, &status, __WALL)) != child)
	{
		if (pid < 0)
		{
			fprintf(stderr, "waitpid failed\n");
			exit(255);
		}
	}
	return status;
}

#ifdef __x86_64__
/* Workaround for openSUSE 13.2 (see below).  */
__attribute__((noreturn)) static void linux3_workaround()
{
	static struct { uint32_t rip, cs; } ljmp_to;
	uint32_t scratch;
	ljmp_to.rip = elks_cpu.regs.xip;
	ljmp_to.cs = elks_cpu.regs.xcs;
	__asm volatile("movl %%ds, %0; movl %0, %%ss; xorl %0, %0; ljmpl *%1"
		       : "=&r" (scratch) : "m" (ljmp_to) : "memory");
	__builtin_unreachable();
}
#endif

void run_elks()
{
	/*
	 *	Execute 8086 code for a while.
	 */
	pid_t child = elks_cpu.child;
	int status;
	if (!child)
	{
		child = clone(child_idle, elks_data_base + elks_cpu.regs.xsp,
			      CLONE_FILES | CLONE_FS | CLONE_IO | CLONE_VM,
			      NULL);
		if (child <= 0)
		{
			fprintf(stderr, "clone call failed\n");
			exit(255);
		}
		dbprintf(("Created child thread %ld\n", (long)child));
		elks_cpu.child = child;
		if (ptrace(PTRACE_ATTACH, child, NULL, NULL) != 0)
		{
			int err = errno;
			fprintf(stderr, "ptrace(PTRACE_ATTACH ...) failed\n");
			fprintf(stderr, "%s\n", strerror(errno));
			exit(255);
		}
		wait_for_child();
	}
	if (ptrace(PTRACE_SETREGS, child, NULL, &elks_cpu.regs) != 0)
	{
		fprintf(stderr, "ptrace(PTRACE_SETREGS ...) failed\n");
		exit(255);
	}
#ifdef __x86_64__
	/*
	 * On Linux 3.16.6 for x86-64 --- as used by openSUSE 13.2 ---
	 * ptrace(PTRACE_SETREGS, ...) does not properly set .cs and .ss in
	 * the `struct user_regs_struct'.
	 *
	 * To overcome this, after PTRACE_SETREGS, first do a sanity check
	 * for the correct .cs and .ss values here.  If they are wrong,
	 * arrange for the child to start at a trampoline which will jump to
	 * the true ELKS program entry point.
	 */
	{
		struct user_regs_struct tr_r;
		if (ptrace(PTRACE_GETREGS, child, NULL, &tr_r) != 0)
		{
			fprintf(stderr,"ptrace(PTRACE_GETREGS ...) failed\n");
			exit(255);
		}
		if (tr_r.xcs != elks_cpu.regs.xcs ||
		    tr_r.xss != elks_cpu.regs.xss)
		{
			tr_r.xip = (uintptr_t)linux3_workaround;
			if (ptrace(PTRACE_SETREGS, child, NULL, &tr_r) != 0)
			{
				fprintf(stderr, "ptrace(PTRACE_SETREGS ...) "
						"failed\n");
				exit(255);
			}
		}
	}
#endif
	if (ptrace(PTRACE_SYSEMU, child, NULL, NULL) != 0)
	{
		fprintf(stderr, "ptrace(PTRACE_SYSEMU ...) failed\n");
		exit(255);
	}
	status = wait_for_child();
	if (ptrace(PTRACE_GETREGS, child, NULL, &elks_cpu.regs) != 0)
	{
		fprintf(stderr, "ptrace(PTRACE_GETREGS ...) failed\n");
		exit(255);
	}
	dbprintf(("%#lx:%#lx\n", elks_cpu.regs.xcs, elks_cpu.regs.xip));
	if (WIFSIGNALED(status))
		raise(WTERMSIG(status));
	else if (WIFSTOPPED(status))
	{
		siginfo_t si;
		if (WSTOPSIG(status) != SIGTRAP
		  || ptrace(PTRACE_GETSIGINFO, child, NULL, &si) != 0
		  || (si.si_code != SIGTRAP
		      && si.si_code != (SIGTRAP | 0x80)))
			raise(WSTOPSIG(status));
		else
		{	/* this is a syscall-stop */
			elks_cpu.regs.xax = elks_cpu.regs.orig_xax;
			elks_take_interrupt(0x80);
		}
	}
	else if (!WIFCONTINUED(status))
	{
		fprintf(stderr, "Unknown return value from waitpid\n");
		exit(255);
	}
}

void build_stack(char ** argv, char ** envp, size_t argv_envp_bytes)
{
	char **p;
	size_t argv_count = 0, envp_count = 0;
	unsigned short * pip;
	unsigned short pcp;

	/* We need to calculate argv_count and envp_count again... */
	for(p=argv; *p; p++)
	   argv_count++;

	for(p=envp; *p; p++)
	   envp_count++;

	/* Allocate stack space in ELKS memory for argv and envp */
	elks_cpu.regs.xsp -= argv_envp_bytes;

	/* Make sp aligned on a 2-byte boundary */
	if ((elks_cpu.regs.xsp & 1) != 0)
		--elks_cpu.regs.xsp;

	/* Now copy in the strings */
	pip=ELKS_PTR(unsigned short, elks_cpu.regs.xsp);
	pcp=elks_cpu.regs.xsp+2*(1+argv_count+1+envp_count+1);

	*pip++ = argv_count;
	for(p=argv; *p; p++)
	{
	   *pip++ = pcp;
	   strcpy(ELKS_PTR(char, pcp), *p);
	   pcp += strlen(*p)+1;
	}
	*pip++ = 0;

	for(p=envp; *p; p++)
	{
	   *pip++ = pcp;
	   strcpy(ELKS_PTR(char, pcp), *p);
	   pcp += strlen(*p)+1;
	}
	*pip++ = 0;
}

int
main(int argc, char *argv[], char *envp[])
{
	int fd;
	struct stat st;
	int ruid, euid, rgid, egid;
	int pg_sz;
	int err;
	size_t argv_envp_bytes;

	if(argc<=1)
	{
		fprintf(stderr,"elksemu {cmd args..... | -t}\n");
		exit(1);
	}
	/* If -t is given, do a quick test to see if our Linux host supports
	 * running 16-bit protected mode code. */
	else if (argc==2 && strcmp(argv[1], "-t")==0)
	{
		elks_init();
		fprintf(stderr, "Yes, this Linux host supports elksemu\n");
		exit(0);
	}

	/* This uses the _real_ user ID If the file is exec only that's */
	/* ok cause the suid root will override.  */
	/* BTW, be careful here, security problems are possible because of 
	 * races if you change this. */

	if( access(argv[1], X_OK) < 0
	  || (fd=open(argv[1], O_RDONLY)) < 0
	  || fstat(fd, &st) < 0
	  )
	{
		perror(argv[1]);
		exit(1);
	}

	/* Check the suid bits ... */
	ruid = getuid(); rgid = getgid();
	euid = ruid; egid = rgid;
	if( st.st_mode & S_ISUID ) euid = st.st_uid;
	if( st.st_mode & S_ISGID ) egid = st.st_gid;

	/* Set the _real_ permissions, or revoke superuser priviliages */
	setregid(rgid, egid);
	setreuid(ruid, euid);

	dbprintf(("ELKSEMU\n"));
	elks_init();
	elks_pid_init();

	pg_sz = getpagesize();
	if (pg_sz < 0)
	{
		fprintf(stderr, "Cannot get system page size\n");
		exit(255);
	}

	/* The Linux vm will deal with not allocating the unused pages */
	elks_base = mmap(NULL, 0x30000 + pg_sz,
			  PROT_EXEC|PROT_READ|PROT_WRITE,
			  MAP_ANON|MAP_PRIVATE|MAP_32BIT, 
			  -1, 0);
	if( elks_base == MAP_FAILED ||
	    (uintptr_t)elks_base >= 0x100000000ull - (0x30000 + pg_sz) )
	{
		fprintf(stderr, "Elks memory is at an illegal address\n");
		exit(255);
	}
	mprotect(elks_base + 0x30000, pg_sz, PROT_NONE);
	
	argv_envp_bytes = count_argv_envp_bytes(argv + 1, envp);

	if (argv_envp_bytes > 0xffffu)
	{
		fprintf(stderr, "Not enough space for argv and envp!\n");
		exit(255);
	}

	if((err = load_elks(fd, (uint16_t)argv_envp_bytes)) < 0)
	{
		if (err == -ENOEXEC)
			fprintf(stderr, "Not a elks binary.\n");
		else
			fprintf(stderr, "Unsupported elks binary (%s).\n",
				strerror(-err));
		exit(1);
	}
	
	close(fd);

	build_stack(argv + 1, envp, argv_envp_bytes);

	while(1)
		run_elks();
}

#ifdef DEBUG
void db_printf(const char * fmt, ...)
{
static FILE * db_fd = 0;
  va_list ptr;
  int rv;
  if( db_fd == 0 )
  {
     db_fd = fopen("/tmp/ELKS_log", "a");
     if( db_fd == 0 ) db_fd = stderr;
     setbuf(db_fd, 0);
  }
  fprintf(db_fd, "%d: ", getpid());
  va_start(ptr, fmt);
  rv = vfprintf(db_fd,fmt,ptr);
  va_end(ptr);
  fflush(db_fd);
}
#endif
