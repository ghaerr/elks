/*
 *	Definitions for emulating ELKS
 */

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/user.h>

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
#define ELKS_F_GETLK	5
#define ELKS_F_SETLK	6
#define ELKS_F_SETLKW	7

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

#define ELKS_PTR(_t,x)	  ((_t *) (elks_data_base+((x)&0xFFFFU)) )
#define ELKS_PEEK(_t,x)	(*((_t *) (elks_data_base+((x)&0xFFFFU)) ))
#define ELKS_POKE(_t,x,_v)	\
		(*((_t *) (elks_data_base+((x)&0xFFFFU)) ) = (_v))

struct elks_cpu_s
{
	struct user_regs_struct regs;
	pid_t child;
};

#ifdef __x86_64__
#define xax		rax
#define xbx		rbx
#define xcx		rcx
#define xdx		rdx
#define xsp		rsp
#define xbp		rbp
#define xsi		rsi
#define xdi		rdi
#define xip		rip
#define xcs		cs
#define xds		ds
#define xes		es
#define xss		ss
#define orig_xax	orig_rax
#else
#define xax		eax
#define xbx		ebx
#define xcx		ecx
#define xdx		edx
#define xsp		esp
#define xbp		ebp
#define xsi		esi
#define xdi		edi
#define xip		eip
#define orig_xax	orig_eax
#endif

extern const char *emu_prog;
extern unsigned char * elks_base, *elks_data_base;
extern uint16_t brk_at;
extern volatile struct elks_cpu_s elks_cpu;

void db_printf(const char *, ...)
    __attribute__((format(printf,1,2)));
int elks_syscall(void);
void minix_syscall(void);
void elks_pid_init(void);
pid_t elks_to_linux_pid(int16_t);
int16_t linux_to_elks_pid(pid_t);

int reboot(int magic1, int magic2, int magic3);
