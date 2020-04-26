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
	uint16_t est_dev;
	uint32_t est_inode;
	uint16_t est_mode;
	uint16_t est_nlink;
	uint16_t est_uid;
	uint16_t est_gid;
	uint16_t est_rdev;
	int32_t est_size;
	int32_t est_atime;
	int32_t est_mtime;
	int32_t est_ctime;
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
#define ELKS_SPLITID	0x04200301L
#define ELKS_SPLITID_AHISTORICAL 0x04300301L
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
extern uint16_t brk_at;
extern volatile struct elks_cpu_s elks_cpu;

void db_printf(const char *, ...);
int elks_syscall(void);
void minix_syscall(void);

int reboot(int magic1, int magic2, int magic3);
