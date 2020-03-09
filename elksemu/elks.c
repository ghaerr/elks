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
unsigned char *elks_base, *elks_data_base;	/* Paragraph aligned */
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
	struct user_desc cs_desc, ds_desc;
	memset(ldt, 0, sizeof ldt);
	int cs_idx = 0, ds_idx, ldt_count = modify_ldt(0, ldt, sizeof ldt);
	unsigned cs, ds;
	if (ldt_count < 0)
		ldt_count = 0;
	while (cs_idx < ldt_count && ldt[cs_idx] != 0)
		++cs_idx;
	ds_idx = cs_idx + 1;
	while (ds_idx < ldt_count && ldt[ds_idx] != 0)
		++ds_idx;
	if (cs_idx >= 8192 || ds_idx >= 8192)
	{
		fprintf(stderr, "No free LDT descriptors for text and data "
				"segments\n");
		exit(255);
	}
	elks_cpu.regs.xcs = cs = cs_idx * 8 + 7;
	elks_cpu.regs.xds = elks_cpu.regs.xes = elks_cpu.regs.xss
			  = ds = ds_idx * 8 + 7;
	dbprintf(("LDT descriptor for text is %#x\n", cs));
	dbprintf(("LDT descriptor for data is %#x\n", ds));
	/* Try to place dummy descriptors in the LDT */
	memset(&cs_desc, 0, sizeof cs_desc);
	cs_desc.entry_number = cs_idx;
	cs_desc.contents = MODIFY_LDT_CONTENTS_CODE;
	cs_desc.useable = 1;
	memset(&ds_desc, 0, sizeof ds_desc);
	ds_desc.entry_number = ds_idx;
	ds_desc.contents = MODIFY_LDT_CONTENTS_DATA;
	ds_desc.useable = 1;
	if (modify_ldt(1, &cs_desc, sizeof cs_desc) != 0
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


static int load_elks(int fd)
{
	/* Load the elks binary image and set it up in the text and data
	   segments. Load CS and DS/SS according to image type. Allocate
	   data segment according to a.out total field */
	struct elks_exec_hdr mh;
	struct user_desc cs_desc, ds_desc;
	if(read(fd, &mh,sizeof(mh))!=sizeof(mh))
		return -ENOEXEC;
	if(mh.hlen!=EXEC_HEADER_SIZE)
		return -ENOEXEC;
	if(mh.type!=ELKS_SPLITID && mh.type!=ELKS_SPLITID_AHISTORICAL)
		return -ENOEXEC;
#ifdef DEBUG
	fprintf(stderr,"Linux-86 binary - %lX. tseg=%ld dseg=%ld bss=%ld\n",
		mh.type,mh.tseg,mh.dseg,mh.bseg);
#endif
	if (mh.total == 0)
		mh.total = mh.dseg + mh.bseg + 0x4000ul;
	if (mh.tseg > 0xfffful || mh.dseg > 0xfffful
	    || mh.bseg > 0xfffful - mh.dseg || mh.entry >= mh.tseg
	    || mh.total > 0xfff0ul || mh.total <= mh.dseg + mh.bseg)
	{
		fprintf(stderr, "Bogus a.out headers\n");
		exit(1);
	}
	mh.total = (mh.total + 0xful) & 0xfff0ul;
	if(read(fd,elks_base,mh.tseg)!=mh.tseg)
		return -ENOEXEC;
	elks_data_base=elks_base+65536;
	if(read(fd,elks_data_base,mh.dseg)!=mh.dseg)
		return -ENOEXEC;
	memset(elks_data_base+mh.dseg,0, mh.bseg);
	/*
	 *	Really set up the LDT descriptors
	 */

	memset(&cs_desc, 0, sizeof cs_desc);
	memset(&ds_desc, 0, sizeof ds_desc);
	cs_desc.entry_number = elks_cpu.regs.xcs / 8;
	cs_desc.base_addr = (uintptr_t)elks_base;
	cs_desc.limit = mh.tseg - 1;
	cs_desc.contents = MODIFY_LDT_CONTENTS_CODE;
	cs_desc.seg_not_present = mh.tseg == 0;
	ds_desc.entry_number = elks_cpu.regs.xss / 8;
	ds_desc.base_addr = (uintptr_t)elks_data_base;
	ds_desc.limit = mh.total - 1;
	ds_desc.contents = MODIFY_LDT_CONTENTS_DATA;
	ds_desc.seg_not_present = mh.dseg == 0 && mh.bseg == 0;
	if (modify_ldt(1, &cs_desc, sizeof cs_desc) != 0
	    || modify_ldt(1, &ds_desc, sizeof ds_desc) != 0)
	{
		fprintf(stderr, "Cannot set LDT descriptors for text and data "
				"segments\n");
		exit(255);
	}
	elks_cpu.regs.xsp = mh.total;	/* Args stacked later */
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

void build_stack(char ** argv, char ** envp)
{
	char **p;
	int argv_len=0, argv_count=0;
	int envp_len=0, envp_count=0;
	int stack_bytes;
	unsigned short * pip;
	unsigned short pcp;

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
	stack_bytes = 2				/* argc */
	            + argv_count * 2 + 2	/* argv */
		    + argv_len
		    + envp_count * 2 + 2	/* envp */
		    + envp_len;

	/* Allocate it */
	elks_cpu.regs.xsp -= stack_bytes;

#ifdef DEBUG
	fprintf(stderr, "Argv = (%d,%d), Envp=(%d,%d), stack=%d\n",
		argv_count, argv_len, envp_count, envp_len, stack_bytes);
#endif

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

	if(argc<=1)
	{
		fprintf(stderr,"elksemu cmd args.....\n");
		exit(1);
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

	pg_sz = getpagesize();
	if (pg_sz < 0)
	{
		fprintf(stderr, "Cannot get system page size\n");
		exit(255);
	}

	/* The Linux vm will deal with not allocating the unused pages */
	elks_base = mmap(NULL, 0x20000 + pg_sz,
	                  PROT_EXEC|PROT_READ|PROT_WRITE,
			  MAP_ANON|MAP_PRIVATE|MAP_32BIT,
			  -1, 0);
	if( (intptr_t)elks_base < 0 || (uintptr_t)elks_base >= 0x100000000ull)
	{
		fprintf(stderr, "Elks memory is at an illegal address\n");
		exit(255);
	}
	mprotect(elks_base + 0x20000, pg_sz, PROT_NONE);

	if(load_elks(fd) < 0)
	{
		fprintf(stderr,"Not a elks binary.\n");
		exit(1);
	}

	close(fd);

	build_stack(argv+1, envp);

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
