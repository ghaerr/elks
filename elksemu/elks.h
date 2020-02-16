/*
 *	Definitions for emulating ELKS
 */

#include <stdint.h>
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
	unsigned short est_dev;
	unsigned short est_inode;
	unsigned short est_mode;
	unsigned short est_nlink;
	unsigned short est_uid;
	unsigned short est_gid;
	unsigned short est_rdev;
	int est_size;
	int est_atime;
	int est_mtime;
	int est_ctime;
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
 
#define EXEC_HEADER_SIZE	32

struct elks_exec_hdr
{
	uint32_t type;
#define ELKS_COMBID	0x04100301L
#define ELKS_SPLITID	0x04300301L	
	uint32_t hlen;
	uint32_t tseg;
	uint32_t dseg;
	uint32_t bseg;
	uint32_t entry;
	uint32_t total;
	uint32_t unused2; 
};

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

extern unsigned char * elks_base, *elks_data_base;
extern unsigned short brk_at;
extern volatile struct elks_cpu_s elks_cpu;

void db_printf(const char *, ...);
int elks_syscall(void);
void minix_syscall(void);
