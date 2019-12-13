/*
 *	ELKSEMU	An emulator for Linux8086 binaries.
 *
 *	VM86 is used to process all the 8086 mode code.
 *	We trap up to 386 mode for system call emulation and naughties. 
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/vm86.h>
#include <sys/mman.h>
#include "elks.h" 

#ifdef __BCC__
#define OLD_LIBC_VERSION
#endif

volatile struct vm86_struct elks_cpu;
unsigned char *elks_base;	/* Paragraph aligned */

#ifdef DEBUG
#define dbprintf(x) db_printf x
#else
#define dbprintf(x) 
#endif

static void elks_init()
{
	elks_cpu.screen_bitmap=0;
	elks_cpu.cpu_type = CPU_286;
	/*
	 *	All INT xx calls are trapped.
	 */
	memset((void *)&elks_cpu.int_revectored,0xFF, sizeof(elks_cpu.int_revectored));
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
	
	dbprintf(("syscall AX=%x BX=%x CX=%x DX=%x\n",
		(unsigned short)elks_cpu.regs.eax,
		(unsigned short)elks_cpu.regs.ebx,
		(unsigned short)elks_cpu.regs.ecx,
		(unsigned short)elks_cpu.regs.edx));
		
	elks_cpu.regs.eax = elks_syscall();
	dbprintf(("elks syscall returned %d\n", elks_cpu.regs.eax));
	/* Finally return to vm86 state */
}


static int load_elks(int fd)
{
	/* Load the elks binary image and set it up in a suitable VM86 segment. Load CS and DS/SS
	   according to image type. chmem is ignored we always use 64K segments */
	struct elks_exec_hdr mh;
	unsigned char *dsp;
	if(read(fd, &mh,sizeof(mh))!=sizeof(mh))
		return -ENOEXEC;
	if(mh.hlen!=EXEC_HEADER_SIZE)
		return -ENOEXEC;
	if(mh.type!=ELKS_COMBID&&mh.type!=ELKS_SPLITID)
		return -ENOEXEC;
#ifdef DEBUG
	fprintf(stderr,"Linux-86 binary - %lX. tseg=%ld dseg=%ld bss=%ld\n",
		mh.type,mh.tseg,mh.dseg,mh.bseg);
#endif
	if(read(fd,elks_base,mh.tseg)!=mh.tseg)
		return -ENOEXEC;
	if(mh.type==ELKS_COMBID)
		dsp=elks_base+mh.tseg;
	else
		dsp=elks_base+65536;
	if(read(fd,dsp,mh.dseg)!=mh.dseg)
		return -ENOEXEC;
	memset(dsp+mh.dseg,0, mh.bseg);
	/*
	 *	Load the VM86 registers
	 */
	 
	if(mh.type==ELKS_COMBID)
		dsp=elks_base;
	elks_cpu.regs.ds=PARAGRAPH(dsp);
	elks_cpu.regs.es=PARAGRAPH(dsp);
	elks_cpu.regs.ss=PARAGRAPH(dsp);
	elks_cpu.regs.esp=65536; 	/* Args stacked later */
	elks_cpu.regs.cs=PARAGRAPH(elks_base);
	elks_cpu.regs.eip=0;		/* Run from 0 */
	
	/*
	 *	Loaded, check for sanity.
	 */
	if( dsp != ELKS_PTR(unsigned char, 0) )
	{
	   printf("Error VM86 problem %lx!=%lx (Is DS > 16 bits ?)\n",
	           (long)dsp, (long)ELKS_PTR(char, 0));
	   exit(0);
	}

	return 0;
}

#ifndef OLD_LIBC_VERSION
  /*
   *  recent versions of libc have changed the proto for vm86()
   *  for now I'll just override ...
   */
#define OLD_SYS_vm86  113
#define NEW_SYS_vm86  166

static inline int vm86_mine(struct vm86_struct* v86)
{
	int __res;
	__asm__ __volatile__("int $0x80\n"
	:"=a" (__res):"a" ((int)OLD_SYS_vm86), "b" ((int)v86));
	return __res;
} 
#endif

void run_elks()
{
	/*
	 *	Execute 8086 code for a while.
	 */
#ifndef OLD_LIBC_VERSION
	int err=vm86_mine((struct vm86_struct*)&elks_cpu);
#else
	int err=vm86((struct vm86_struct*)&elks_cpu);
#endif
	switch(VM86_TYPE(err))
	{
		/*
		 *	Signals are just re-starts of emulation (yes the
		 *	handler might alter elks_cpu)
		 */
		case VM86_SIGNAL:
			break;
		case VM86_UNKNOWN:
			fprintf(stderr, "VM86_UNKNOWN returned\n");
			exit(1);
		case VM86_INTx:
			elks_take_interrupt(VM86_ARG(err));
			break;
		case VM86_STI:
			fprintf(stderr, "VM86_STI returned\n");
			break;	/* Shouldnt be seen */
		default:
			fprintf(stderr, "Unknown return value from vm86\n");
			exit(1);
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

	/* How much space for envp */
	for(p=envp; *p; p++)
	{
	   envp_count++; envp_len += strlen(*p)+1;
	}

	/* tot it all up */
	stack_bytes = 2				/* argc */
	            + argv_count * 2 + 2	/* argv */
		    + argv_len
		    + envp_count * 2 + 2	/* envp */
		    + envp_len;

	/* Allocate it */
	elks_cpu.regs.esp -= stack_bytes;

/* Sanity check 
	printf("Argv = (%d,%d), Envp=(%d,%d), stack=%d\n",
	        argv_count, argv_len, envp_count, envp_len, stack_bytes);
*/

	/* Now copy in the strings */
	pip=ELKS_PTR(unsigned short, elks_cpu.regs.esp);
	pcp=elks_cpu.regs.esp+2*(1+argv_count+1+envp_count+1);

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

	/* The Linux vm will deal with not allocating the unused pages */
#if __AOUT__
#if __GNUC__
	/* GNU malloc will align to 4k with large chunks */
	elks_base = malloc(0x20000);
#else
	/* But others won't */
	elks_base = malloc(0x20000+4096);
	elks_base = (void*) (((int)elks_base+4095) & -4096);
#endif
#else
	/* For ELF first 128M is unmapped, it needs to be mapped manually */
	elks_base = mmap((void*)0x10000, 0x20000,
	                  PROT_EXEC|PROT_READ|PROT_WRITE,
			  MAP_ANON|MAP_PRIVATE|MAP_FIXED, 
			  0, 0);
#endif
	if( (long)elks_base < 0 || (long)elks_base >= 0xE0000 )
	{
		fprintf(stderr, "Elks memory is at an illegal address\n");
		exit(255);
	}
	
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
}
#endif
