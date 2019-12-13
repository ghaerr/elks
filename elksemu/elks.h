/*
 *	Definitions for emulating ELKS
 */
 
#define ELKS_CS_OFFSET		0
#define ELKS_DS_OFFSET		0		/* For split I/D */

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
};


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
	unsigned long type;
#define ELKS_COMBID	0x04100301L
#define ELKS_SPLITID	0x04200301L	
	unsigned long hlen;
	unsigned long tseg;
	unsigned long dseg;
	unsigned long bseg;
	unsigned long unused;
	unsigned long chmem;
	unsigned long unused2; 
};

#define PARAGRAPH(x)	(((unsigned long)(x))>>4)
#define ELKS_DSEG(x)	((unsigned char *)(((x)&0xFFFF)+(elks_cpu.regs.ds<<4)))

#define ELKS_PTR(_t,x)	  ((_t *) ((elks_cpu.regs.ds<<4)+((x)&0xFFFF)) )
#define ELKS_PEEK(_t,x)	(*((_t *) ((elks_cpu.regs.ds<<4)+((x)&0xFFFF)) ))
#define ELKS_POKE(_t,x,_v)	\
		(*((_t *) ((elks_cpu.regs.ds<<4)+((x)&0xFFFF)) ) = (_v))

extern unsigned char * elks_base;
extern volatile struct vm86_struct elks_cpu;

void db_printf(const char *, ...);
int elks_syscall(void);
void minix_syscall(void);
