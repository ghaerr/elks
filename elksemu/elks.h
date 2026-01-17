/*
 *	Definitions for emulating ELKS
 */

#include <stddef.h>
#include <stdint.h>
#if USE_VM86
#include <sys/vm86.h>
#elif USE_X86EMU
#include <x86emu.h>
#elif USE_PTRACE
#include <sys/types.h>
#include <sys/user.h>
#endif

#define HZ			100

#define ELKS_SIG_IGN		(-1)
#define ELKS_SIG_DFL		0

#define WRITE_USPACE		0
#define READ_USPACE		1

#if !__ELF__
#define __AOUT__		1
#endif

/*
 *	Minix view of stat(). We have to squash a bit here and give
 *	wrong values with inode >65535 etc
 */

struct elks_stat
{
	uint16_t est_dev;
	uint32_t est_inode;
	uint16_t est_mode;
	uint16_t est_nlink;
	uint16_t est_uid;
	uint16_t est_gid;
	uint16_t est_rdev;
	int32_t est_size;
	uint32_t est_atime;
	uint32_t est_mtime;
	uint32_t est_ctime;
} __attribute__((packed));


/*
 *	Minix ioctl list
 */

#define 	ELKS_TIOCGETP	(('t'<<8)|8)
#define		ELKS_TIOCSETP	(('t'<<8)|9)
#define		ELKS_TIOCGETC	(('t'<<8)|18)
#define		ELKS_TIOCSETC	(('t'<<8)|17)
#define 	ELKS_TIOCFLUSH	(('t'<<8)|16)

/*
 *	fcntl list
 */

#define ELKS_F_DUPFD	0
#define ELKS_F_GETFD	1
#define ELKS_F_SETFD	2
#define ELKS_F_GETFL	3
#define ELKS_F_SETFL	4

/*
 *	Elks binary formats
 */

#define EXEC_MINIX_HDR_SIZE	sizeof(struct minix_exec_hdr)
#define SUPL_RELOC_HDR_SIZE	offsetof(struct elks_supl_hdr, esh_ftseg)
#define EXEC_RELOC_HDR_SIZE	(EXEC_MINIX_HDR_SIZE + SUPL_RELOC_HDR_SIZE)
#define SUPL_FARTEXT_HDR_SIZE	sizeof(struct elks_supl_hdr)
#define EXEC_FARTEXT_HDR_SIZE	(EXEC_MINIX_HDR_SIZE + SUPL_FARTEXT_HDR_SIZE)

struct __attribute__((packed)) minix_exec_hdr
{
	uint32_t type;
#define ELKS_COMBID	0x04100301L
#define ELKS_SPLITID	0x04200301L
#define ELKS_SPLITID_AHISTORICAL 0x04300301L
	uint8_t hlen;
	uint8_t reserved1;
	uint16_t version;
	uint16_t tseg, reserved2;
	uint16_t dseg, reserved3;
	uint16_t bseg, reserved4;
	uint32_t entry;
	uint16_t chmem;
	uint16_t minstack;
	uint32_t syms;
};

struct __attribute__((packed)) elks_supl_hdr
{
	/* optional fields */
	uint32_t msh_trsize;
	uint32_t msh_drsize;
	uint32_t msh_tbase;
	uint32_t msh_dbase;
	/* even more optional fields */
	uint16_t esh_ftseg, esh_reserved1;
	uint32_t esh_ftrsize;
	uint32_t esh_reserved2, esh_reserved3;
};

struct __attribute__((packed)) minix_reloc
{
	uint32_t r_vaddr;
	uint16_t r_symndx;
	uint16_t r_type;
};

/* r_type values */
#define R_SEGWORD	80

/* special r_symndx values */
#define S_ABS		((uint16_t)-1U)
#define S_TEXT		((uint16_t)-2U)
#define S_DATA		((uint16_t)-3U)
#define S_BSS		((uint16_t)-4U)
/* for ELKS medium memory model support */
#define S_FTEXT		((uint16_t)-5U)

#define INIT_HEAP	4096
#define INIT_STACK	4096

#if USE_VM86
#define PARAGRAPH(x) (((unsigned int)(x))>>4)
#define ELKS_PTR(_t,x)	  ((_t *) ((elks_cpu.xds<<4)+((x)&0xFFFFU)) )
#define ELKS_PEEK(_t,x)	(*((_t *) ((elks_cpu.xds<<4)+((x)&0xFFFFU)) ))
#define ELKS_POKE(_t,x,_v)	\
		(*((_t *) ((elks_cpu.xds<<4)+((x)&0xFFFFU)) ) = (_v))
#elif USE_X86EMU
/* what the VM understands as the base address */
#define ELKS_BASE 0x10000
#define PARAGRAPH(x) ((ELKS_BASE + ((x) - elks_base))>>4)
#define ELKS_PTR(_t,x)	  ((_t *) (elks_base-ELKS_BASE+(elks_cpu.xds<<4)+((x)&0xFFFFU)) )
#define ELKS_PEEK(_t,x)	(*((_t *) (elks_base-ELKS_BASE+(elks_cpu.xds<<4)+((x)&0xFFFFU)) ))
#define ELKS_POKE(_t,x,_v)	\
		(*((_t *) (elks_base-ELKS_BASE+(elks_cpu.xds<<4)+((x)&0xFFFFU)) ) = (_v))
#elif USE_PTRACE
#define ELKS_PTR(_t,x)	  ((_t *) (elks_data_base+((x)&0xFFFFU)) )
#define ELKS_PEEK(_t,x)	(*((_t *) (elks_data_base+((x)&0xFFFFU)) ))
#define ELKS_POKE(_t,x,_v)	\
		(*((_t *) (elks_data_base+((x)&0xFFFFU)) ) = (_v))
#endif

#if USE_VM86

typedef struct vm86plus_struct elks_cpu_t;

#define xax regs.eax
#define xbx regs.ebx
#define xcx regs.ecx
#define xdx regs.edx
#define xsp regs.esp
#define xbp regs.ebp
#define xsi regs.esi
#define xdi regs.edi
#define xip regs.eip
#define xcs regs.cs
#define xds regs.ds
#define xes regs.es
#define xfs regs.fs
#define xgs regs.gs
#define xss regs.ss
#define orig_xax regs.orig_eax
#define xflags regs.eflags

#elif USE_X86EMU

struct elks_cpu_s
{
	struct x86emu_s * regs;
	unsigned int orig_eax;
};
typedef struct elks_cpu_s elks_cpu_t;

#define xax regs->x86.R_EAX
#define xbx regs->x86.R_EBX
#define xcx regs->x86.R_ECX
#define xdx regs->x86.R_EDX
#define xsp regs->x86.R_ESP
#define xbp regs->x86.R_EBP
#define xsi regs->x86.R_ESI
#define xdi regs->x86.R_EDI
#define xip regs->x86.R_EIP
#define xcs regs->x86.R_CS
#define xds regs->x86.R_DS
#define xes regs->x86.R_ES
#define xfs regs->x86.R_FS
#define xgs regs->x86.R_GS
#define xss regs->x86.R_SS
#define orig_xax orig_eax
#define xflags regs->x86.R_EFLG

#elif USE_PTRACE

struct elks_cpu_s
{
	struct user_regs_struct regs;
	pid_t child;
};
typedef struct elks_cpu_s elks_cpu_t;

#ifdef __x86_64__
#define xax		regs.rax
#define xbx		regs.rbx
#define xcx		regs.rcx
#define xdx		regs.rdx
#define xsp		regs.rsp
#define xbp		regs.rbp
#define xsi		regs.rsi
#define xdi		regs.rdi
#define xip		regs.rip
#define xcs		regs.cs
#define xds		regs.ds
#define xes		regs.es
#define xfs		regs.fs
#define xgs		regs.gs
#define xss		regs.ss
#define orig_xax	regs.orig_rax
#else
#define xax		regs.eax
#define xbx		regs.ebx
#define xcx		regs.ecx
#define xdx		regs.edx
#define xsp		regs.esp
#define xbp		regs.ebp
#define xsi		regs.esi
#define xdi		regs.edi
#define xip		regs.eip
#define xcs		regs.xcs
#define xds		regs.xds
#define xes		regs.xes
#define xfs		regs.xfs
#define xgs		regs.xgs
#define xss		regs.xss
#define orig_xax	regs.orig_eax
#endif
#define xflags eflags
#endif

extern const char *emu_prog;
extern unsigned char * elks_base, *elks_data_base;
extern uint16_t brk_at;
extern volatile elks_cpu_t elks_cpu;

void db_printf(const char *, ...)
    __attribute__((format(printf,1,2)));
int elks_syscall(void);
void minix_syscall(void);
void elks_pid_init(void);
pid_t elks_to_linux_pid(int16_t);
int16_t linux_to_elks_pid(pid_t);

int reboot(int magic1, int magic2, int magic3, void *args);
