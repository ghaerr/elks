/*
 * System calls are mostly pretty easy as the emulator is tightly bound to
 * the elks task.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/vm86.h>
#include <sys/times.h>
#include <utime.h>
#include <termios.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/time.h>
#include "elks.h" 

#include "efile.h"

#ifdef DEBUG
#define dbprintf(x) db_printf x
#else
#define dbprintf(x)
#endif

#define sys_signal elks_signal
extern int elks_signal(int bx,int cx,int dx,int di,int si);

/* Forward refs */
static int elks_termios(int bx,int cx,int dx,int di,int si);
static int elks_enosys(int bx,int cx,int dx,int di,int si);

#define DIRCOUNT 20
DIR * dirtab[DIRCOUNT];
int diropen = 0;
static int elks_opendir(char * dname);
static int elks_readdir(int bx,int cx,int dx,int di,int si);
static int elks_closedir(int bx);

/*
 *	Compress a host stat into a elks one. Lose upper bits with wild
 *	abandon. For SYS5.3 this isn't a problem, but machines with 32
 *	bit inodes (BSD, SYS5 with veritas, newest SCO) you lose the top
 *	bits which can confuse a few programs which use inode numbers 
 *	(eg gnu tar).
 */
 
static void squash_stat(struct stat *s, int bx)
{
#if 1	/* Can't use elks_stat, shot in the foot by alignment */

	ELKS_POKE(short, bx+0, s->st_dev);
	ELKS_POKE(short, bx+2, s->st_ino ^ (s->st_ino>>16));
	ELKS_POKE(short, bx+4, s->st_mode);
	ELKS_POKE(short, bx+6, s->st_nlink);
	ELKS_POKE(short, bx+8, s->st_uid);
	ELKS_POKE(short, bx+10, s->st_gid);
	ELKS_POKE(short, bx+12, s->st_rdev);
	ELKS_POKE(long, bx+14, s->st_size);
	ELKS_POKE(long, bx+18, s->st_atime);
	ELKS_POKE(long, bx+22, s->st_mtime);
	ELKS_POKE(long, bx+26, s->st_ctime);
#else
	struct elks_stat * ms = ELKS_PTR(struct elks_stat, bx);
	ms->est_dev=s->st_dev;
	ms->est_inode=(unsigned short)s->st_ino;	/* Bits lost */
	ms->est_mode=s->st_mode;
	ms->est_nlink=s->st_nlink;
	ms->est_uid=s->st_uid;
	ms->est_gid=s->st_gid;
	ms->est_rdev=s->st_rdev;
	ms->est_size=s->st_size;
	ms->est_atime=s->st_atime;
	ms->est_mtime=s->st_mtime;
	ms->est_ctime=s->st_ctime;
#endif
}

/*
 *	Implementation of ELKS syscalls.
 */
 
 
#define sys_exit elks_exit
static int elks_exit(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("exit(%d)\n",bx));
	exit(bx);
}

#define sys_vfork elks_fork
#define sys_fork elks_fork
static int elks_fork(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("fork()\n"));
	/* This is fun 8) - fork the emulator (its easier that way) */
	return fork();
}

#define sys_read elks_read
static int elks_read(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("read(%d, %d, %d)\n",
		bx,cx,dx));
	if( bx >= 10000 && bx < 10000+DIRCOUNT)
		return elks_readdir(bx, cx, dx, di, si);
	if( dx < 0 || dx > 1024 ) dx = 1024;
	return read(bx, ELKS_PTR(void, cx), dx);
}

#define sys_write elks_write
static int elks_write(int bx,int cx,int dx,int di,int si)
{
#ifdef TESTING
	if( dx > 1024 || dx < 0 )
	{
	   dx = 1024;
	   dbprintf(("write(%d, %d, >%d)\n",bx,cx,dx));
	}
	else 
#endif
	{
	   dbprintf(("write(%d, %d, %d)\n",bx,cx,dx));
	}
	return write(bx,ELKS_PTR(void, cx),dx);
}

#define sys_open elks_open
static int elks_open(int bx,int cx,int dx,int di,int si)
{
	struct stat s;

	/* Assumes _all_ flags are the same */
	char *dp=ELKS_PTR(char, bx);
	dbprintf(("open(%s, %d, %d)\n",
		dp,cx,dx));

	/* Nasty hack so /lib/liberror.txt doesn't exist on the host.
	 */
	if (strcmp(dp, "/lib/liberror.txt") == 0 ) {
	   int fd = open("/tmp/liberror.txt", O_CREAT|O_EXCL|O_RDWR, 0666);
	   if (fd < 0) return fd;
	   unlink("/tmp/liberror.txt");
	   write(fd, efile, sizeof(efile));
	   lseek(fd, 0L, 0);
	   return fd;
	}

	if( cx == O_RDONLY )
	{
		if(stat(dp,&s)==-1)
			return -1;
		if( S_ISDIR(s.st_mode) )
			return elks_opendir(dp);
	}

	return open(dp,cx,dx);
}

#define sys_close elks_close
static int elks_close(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("close(%d)\n",bx));
	if( bx >= 10000 && bx < 10000+DIRCOUNT)
		return elks_closedir(bx);
	return close(bx);
}

#define sys_wait4 elks_wait4
static int elks_wait4(int bx,int cx,int dx,int di,int si)
{
	int status;
	unsigned short *tp=ELKS_PTR(unsigned short, cx);
	int r;
	struct rusage use;

	dbprintf(("wait4(%d, %d, %d, %d)\n", bx, cx, dx, di));
	r=wait4((int)(short)bx, &status, dx, &use );

	*tp=status;
	if( di ) memcpy(ELKS_PTR(void, di), &use, sizeof(use));
	return r;
}

#define sys_link elks_link
static int elks_link(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("link(%s,%s)\n", ELKS_PTR(char, bx), ELKS_PTR(char, cx)));
	return link(ELKS_PTR(char, bx),ELKS_PTR(char, cx));
}

#define sys_unlink elks_unlink
static int elks_unlink(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("unlink(%s)\n",ELKS_PTR(char, bx)));
	return unlink(ELKS_PTR(char, bx));
}

#define sys_chdir elks_chdir
static int elks_chdir(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("chdir(%s)\n",ELKS_PTR(char, bx)));
	return chdir(ELKS_PTR(char, bx));
}

#define sys_fchdir elks_fchdir
static int elks_fchdir(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("fchdir(%s)\n",bx));
	return fchdir(bx);
}


#define sys_mknod elks_mknod
static int elks_mknod(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("mknod(%s,%d,%d)\n", ELKS_PTR(char, bx),cx,dx));
	return mknod(ELKS_PTR(char, bx),cx,dx);
}

#define sys_chmod elks_chmod
static int elks_chmod(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("chmod(%s,%d)\n", ELKS_PTR(char, bx),cx));
	return chmod(ELKS_PTR(char, bx), cx);
}

#define sys_chown elks_chown
static int elks_chown(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("chown(%s,%d,%d)\n", ELKS_PTR(char, bx),cx,dx));
	return chown(ELKS_PTR(char, bx),cx,dx);
}

#define sys_brk elks_brk
static int elks_brk(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("brk(%d)\n",bx));	
	if(bx>=elks_cpu.regs.esp)
	{
		errno= 1;	/* Really return -1 */
		return -1;
	}
	return 0;		/* Can't return bx, 0xBAD1 is an error */
}

#define sys_stat elks_stat
static int elks_stat(int bx,int cx,int dx,int di,int si)
{
	struct stat s;
	dbprintf(("stat(%s,%d)\n", ELKS_PTR(char, bx), cx));
	if(stat(ELKS_PTR(char, bx),&s)==-1)
		return -1;
	squash_stat(&s,cx);
	return 0;
}

#define sys_lstat elks_lstat
static int elks_lstat(int bx,int cx,int dx,int di,int si)
{
	struct stat s;
	dbprintf(("lstat(%s,%d)\n", ELKS_PTR(char, bx), cx));
	if(lstat(ELKS_PTR(char, bx),&s)==-1)
		return -1;
	squash_stat(&s,cx);
	return 0;
}

#define sys_lseek elks_lseek
static int elks_lseek(int bx,int cx,int dx,int di,int si)
{
	long l=ELKS_PEEK(long, cx);
	
	dbprintf(("lseek(%d,%ld,%d)\n",bx,l,dx));
	l = lseek(bx,l,dx);
	if( l < 0 ) return -1;
	ELKS_POKE(long, cx, l);
	return 0;
}

#define sys_getpid elks_getpid
static int elks_getpid(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("getpid/getppid()\n"));
	ELKS_POKE(unsigned short, bx, getppid());
	return getpid();
}

#define sys_setuid elks_setuid
static int elks_setuid(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("setuid(%d)\n",bx));
	return setuid(bx);
}

#define sys_getuid elks_getuid
static int elks_getuid(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("get[e]uid()\n"));
	ELKS_POKE(unsigned short, bx, geteuid());
	return getuid();
}

#define sys_alarm elks_alarm
static int elks_alarm(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("alarm(%d)\n",bx&0xFFFF));
	return alarm(bx&0xFFFF);
}

#define sys_fstat elks_fstat
static int elks_fstat(int bx,int cx,int dx,int di,int si)
{
	struct stat s;
	int err;
	dbprintf(("fstat(%d,%d)\n",bx,cx));
	err=fstat(bx,&s);
	squash_stat(&s,cx);
	return err;
}

#define sys_pause elks_pause
static int elks_pause(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("pause()\n"));
	return pause();
}

#define sys_utime elks_utime
static int elks_utime(int bx,int cx,int dx,int di,int si)
{
	unsigned long *up=ELKS_PTR(long, cx);
	struct utimbuf u;
	u.actime=*up++;
	u.modtime=*up;
	return utime(ELKS_PTR(char, bx), &u);
}

#define sys_access elks_access
static int elks_access(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("access(%s,%d)\n",ELKS_PTR(char, bx),cx));
	return access(ELKS_PTR(char, bx),cx);
}

#define sys_sync elks_sync
static int elks_sync(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("sync()\n"));
	sync();
	return 0;
}

#define sys_kill elks_kill
static int elks_kill(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("kill(%d,%d)\n",bx,cx));
	return kill(bx,cx);
}

#define sys_pipe elks_pipe
static int elks_pipe(int bx,int cx,int dx,int di,int si)
{
	unsigned short *dp=ELKS_PTR(unsigned short, bx);
	int p[2];
	int err=pipe(p);
	if(err==-1)
		return err;
	*dp++=p[0];
	*dp=p[1];
	return 0;
}

#define sys_times elks_times
static int elks_times(int bx,int cx,int dx,int di,int si)
{
	struct tms t;
	long clock_ticks=times(&t);
	long *tp=ELKS_PTR(long, bx);
	long *clkt=ELKS_PTR(long, cx);
	*tp++=t.tms_utime;
	*tp++=t.tms_stime;
	*tp++=t.tms_cutime;
	*tp=t.tms_cstime;
	*clkt = clock_ticks;
	return 0;	/* Should be clock_ticks */
}

#define sys_setgid elks_setgid
static int elks_setgid(int bx,int cx,int dx,int di,int si)
{
	return setgid(bx);
}

#define sys_getgid elks_getgid
static int elks_getgid(int bx,int cx,int dx,int di,int si)
{
	ELKS_POKE(unsigned short, bx, getegid());
	return getgid();
}

/*
 * Exec is fun. The Minix user library builds a complete elks stack image.
 * Great except that we need to unpack it all again and do a real exec. If
 * its another elks image then our kernel side binary loader will load
 * elksemu again and we'll take the Unix args and turn them back into a
 * elks stack image.
 * 
 * For now we run elksemu ourselves and do token attempts at binary checking.
 *
 * Of course if the kernel misc module is confiured we could just run the exe.
 */
#define sys_execve elks_execve
static int elks_execve(int bx,int cx,int dx,int di,int si)
{
	int fd;
	int arg_ct,env_ct;
	int ct;
	char **argp, **envp;
	unsigned short *bp;
	unsigned char *base;
	unsigned short *tmp;
	struct elks_exec_hdr mh;
	int is_elks = 1;
	
	dbprintf(("exec(%s,%d,%d)\n",ELKS_PTR(char, bx), cx, dx));

	base=ELKS_PTR(unsigned char, cx);
	bp=ELKS_PTR(unsigned short, cx+2);
	tmp=bp;

	fd=open(ELKS_PTR(char, bx),O_RDONLY);
	if(fd==-1)
	{ errno = ENOENT; return -1; }
	if(read(fd, &mh, sizeof(mh))!=sizeof(mh))
	{
		close(fd);
		errno = ENOEXEC;
		return -1;
	}
	close(fd);
	if(mh.hlen!=EXEC_HEADER_SIZE
	   || (mh.type!=ELKS_COMBID && mh.type!=ELKS_SPLITID))
	   is_elks = 0;

	arg_ct = env_ct = 0;
	while(*tmp++)
		arg_ct++;
	while(*tmp++)
		env_ct++;
	arg_ct+=2;	/* elksemu-path progname arg0...argn */
	argp=malloc(sizeof(char *)*(arg_ct+1));
	envp=malloc(sizeof(char *)*(env_ct+1));
	if(!argp||!envp) { errno = ENOMEM; return -1; }
	ct=0;
	if( is_elks )
	{
	   argp[0]="/usr/bin/elksemu";
	   /* argp[1]=ELKS_PTR(char, bx); */
	   ct=1;
	}
	while(*bp)
		argp[ct++]=ELKS_PTR(char, cx+ *bp++);
	argp[ct]=0;
	bp++;
	ct=0;
	while(*bp)
		envp[ct++]=ELKS_PTR(char, cx+ *bp++);
	envp[ct]=0;
	if( is_elks )
	{
	   argp[1]=ELKS_PTR(char, bx);
	   execve(argp[0],argp,envp);
	}
	else
	   execve(ELKS_PTR(char, bx),argp,envp);
	if( errno == ENOEXEC || errno == EACCES ) return -1;
	perror("elksemu");
	exit(1);
}

#define sys_umask elks_umask
static int elks_umask(int bx,int cx,int dx,int di,int si)
{
	return umask(bx);
}

#define sys_chroot elks_chroot
static int elks_chroot(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("chroot(%s)\n", ELKS_PTR(char, bx)));
	return chroot(ELKS_PTR(char, bx));
}


#define sys_fcntl elks_fcntl
static int elks_fcntl(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("fcntl(%d,%d,%d)\n", bx,cx,dx));
	switch(cx)
	{
		case ELKS_F_GETFD:
			return fcntl(bx,F_GETFD,0);
		case ELKS_F_GETFL:
			return fcntl(bx,F_GETFL,0);
		case ELKS_F_DUPFD:
			return fcntl(bx,F_DUPFD,dx);
		case ELKS_F_SETFD:
			return fcntl(bx,F_SETFD,dx);
		case ELKS_F_SETFL:
			return fcntl(bx,F_SETFL,dx);
		/*
		 *	Fixme: Unpack and process elks file locks 
		 */
		case ELKS_F_GETLK:
		case ELKS_F_SETLK:
		case ELKS_F_SETLKW:
			errno = EINVAL;
			return -1;
	}
	errno = EINVAL;
	return -1;
}

#define sys_dup elks_dup
static int elks_dup(int bx,int cx,int dx,int di,int si)
{
	return dup(bx);
}

#define sys_dup2 elks_dup2
static int elks_dup2(int bx,int cx,int dx,int di,int si)
{
	return dup2(bx, cx);
}

#define sys_rename elks_rename
static int elks_rename(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("rename(%s,%s)\n", ELKS_PTR(char, bx), ELKS_PTR(char, cx)));
	return rename(ELKS_PTR(char, bx), ELKS_PTR(char, cx));
}

#define sys_mkdir elks_mkdir
static int elks_mkdir(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("mkdir(%s,%d)\n", ELKS_PTR(char, bx),cx));
	return mkdir(ELKS_PTR(char, bx),cx);
}

#define sys_rmdir elks_rmdir
static int elks_rmdir(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("rmdir(%s)\n", ELKS_PTR(char, bx)));
	return rmdir(ELKS_PTR(char, bx));
}

#define sys_gettimeofday elks_gettimeofday
static int elks_gettimeofday(int bx,int cx,int dx,int di,int si)
{
	struct timeval tv;
	struct timezone tz;
	int ax;
	dbprintf(("gettimeofday(%d,%d)\n",bx,cx));

	ax = gettimeofday(&tv, &tz);

	if( ax == 0 && bx )
	{
	   ELKS_POKE(long, bx, tv.tv_sec);
	   ELKS_POKE(long, bx+4, tv.tv_usec);
	}
	if( ax == 0 && cx )
	{
	   ELKS_POKE(short, cx, tz.tz_minuteswest);
	   ELKS_POKE(short, cx+2, tz.tz_dsttime);
	}
	return ax?-1:0;
}

#define sys_settimeofday elks_settimeofday
static int elks_settimeofday(int bx,int cx,int dx,int di,int si)
{
	struct timeval tv, *pv = 0;
	struct timezone tz, *pz = 0;
	int ax;
	dbprintf(("settimeofday(%d,%d)\n",bx,cx));

	if( bx )
	{
	   pv = &tv;
	   tv.tv_sec  = ELKS_PEEK(long, bx);
	   tv.tv_usec = ELKS_PEEK(long, bx+4);
	}
	if( cx )
	{
	   pz = &tz;
	   tz.tz_minuteswest = ELKS_PEEK(short, cx);
	   tz.tz_dsttime =     ELKS_PEEK(short, cx+2);
	}

	ax = settimeofday(pv, pz);
	return ax?-1:0;
}

#define sys_nice elks_nice
static int elks_nice(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("nice(%d)\n",bx));
	return nice(bx);
}

#define sys_symlink elks_symlink
static int elks_symlink(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("symlink(%s,%s)\n", ELKS_PTR(char, bx), ELKS_PTR(char, cx)));
	return symlink(ELKS_PTR(char, bx), ELKS_PTR(char, cx));
}

#define sys_readlink elks_readlink
static int elks_readlink(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("readlink(%s,%s,%d)\n",
		ELKS_PTR(char, bx),
		ELKS_PTR(char, cx),
		dx));
	return readlink(ELKS_PTR(char, bx), ELKS_PTR(char, cx), dx);
}

#define sys_ioctl elks_ioctl
static int elks_ioctl(int bx,int cx,int dx,int di,int si)
{
	dbprintf(("ioctl(%d,0x%04x,0x%04x)\n", bx,cx,dx));
	switch((cx>>8)&0xFF)
	{
	case 'T': return elks_termios(bx,cx,dx,di,si);
	default:  return elks_enosys(bx,cx,dx,di,si);
	}
}

#define sys_reboot elks_reboot
static int elks_reboot(int bx,int cx,int dx,int di,int si)
{
   errno = EINVAL;
   if( bx != 0xfee1 || cx != 0xdead ) return -1;

   switch(dx)
   {
   /* graceful shutdown, C-A-D off, kill -? 1 */
   case 0:     return reboot(0xfee1dead, 672274793, 0);
   /* Enable C-A-D */
   case 0xCAD: return reboot(0xfee1dead, 672274793, 0x89abcdef);
   /* Time to die! */
   case 0xD1E: return reboot(0xfee1dead, 672274793, 0x1234567);
   }
   return -1;
}

/****************************************************************************/

static int
elks_opendir(char * dname)
{
	DIR * d;
	int rv;
	for(rv=0; rv<DIRCOUNT; rv++)
		if( dirtab[rv] == 0 )
			break;
	if( rv >= DIRCOUNT ) { errno=ENOMEM; return -1; }
	d = opendir(dname);
	if( d == 0 ) return -1;
	dirtab[rv] = d;
	return 10000+rv;
}

#define sys_readdir elks_readdir
static int elks_readdir(int bx,int cx,int dx,int di,int si)
{
	struct dirent * ent;

	/* Only read _ONE_ _WHOLE_ dirent at a time */
	if( dx != 266 && dx != 1 )
	{
		errno=EINVAL; return -1;
	}
	errno = 0;
	ent = readdir(dirtab[bx-10000]);
	if( ent == 0 ) { if( errno ) { return -1; } else return 0; }

	memcpy(ELKS_PTR(char, cx+10), ent->d_name, ent->d_reclen+1);
	ELKS_POKE(long, cx, ent->d_ino);
	ELKS_POKE(short, cx+8, ent->d_reclen);
	return dx;
}

static int
elks_closedir(int bx)
{
	bx-=10000;
	if( dirtab[bx] ) closedir(dirtab[bx]);
	dirtab[bx] = 0;
	return 0;
}

/****************************************************************************/

static int elks_termios(int bx,int cx,int dx,int di,int si)
{
   int rv = 0;
   switch(cx&0xFF)
   {
   case 0x01: rv = ioctl(bx, TCGETS, ELKS_PTR(void, dx)); break;
   case 0x02: rv = ioctl(bx, TCSETS, ELKS_PTR(void, dx)); break;
   case 0x03: rv = ioctl(bx, TCSETSW, ELKS_PTR(void, dx)); break;
   case 0x04: rv = ioctl(bx, TCSETSF, ELKS_PTR(void, dx)); break;

   case 0x09: rv = ioctl(bx, TCSBRK, dx); break;
   case 0x0A: rv = ioctl(bx, TCXONC, dx); break;
   case 0x0B: rv = ioctl(bx, TCFLSH, dx); break;

   case 0x11: rv = ioctl(bx, TIOCOUTQ, ELKS_PTR(void, dx)); break;
   case 0x1B: rv = ioctl(bx, TIOCINQ, ELKS_PTR(void, dx)); break;

   default: rv = -1; errno = EINVAL; break;
   }
   return rv;
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
#define sys_enosys elks_enosys
static int elks_enosys(int bx,int cx,int dx,int di,int si)
{
	fprintf(stderr, "Function number %d called (%d,%d,%d)\n",
	                 (int)(0xFFFF&elks_cpu.regs.eax),
			 bx, cx, dx);
	errno = ENOSYS;
	return -1;
}

#include "defn_tab.v"
/* * */

typedef int (*funcp)(int, int, int, int, int);

static funcp jump_tbl[] = {
#include "call_tab.v"
   elks_enosys
};
 
int elks_syscall(void)
{
	int r, n;
	int bx=elks_cpu.regs.ebx&0xFFFF;
	int cx=elks_cpu.regs.ecx&0xFFFF;
	int dx=elks_cpu.regs.edx&0xFFFF;
	int di=elks_cpu.regs.edi&0xFFFF;
	int si=elks_cpu.regs.esi&0xFFFF;
	
	errno=0;
	n = (elks_cpu.regs.eax&0xFFFF);
	if( n>= 0 && n< sizeof(jump_tbl)/sizeof(funcp) )
	   r = (*(jump_tbl[n]))(bx, cx, dx, di, si);
	else
		return -ENOSYS;

	if(r>=0)
		return r;
	else
		return -errno;
}
