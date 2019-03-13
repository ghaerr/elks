
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/stat.h>

#include <errno.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

void (* _cleanup)() = 0;  // called by exit()

int errno;
char ** environ;

//-----------------------------------------------------------------------------

extern pid_t _getpid (pid_t *);

pid_t getpid ()
{
	pid_t ppid;
	return _getpid (&ppid);
}

//-----------------------------------------------------------------------------

extern int _lseek (int fd, off_t * posn, int where);

off_t lseek (int fd, off_t posn, int where)
{
	if ( _lseek (fd, &posn, where) < 0) return -1;
	else return posn;
}

/********************** Function getppid ************************************/

#ifdef L_getppid
int getppid()
{
   int ppid;
   __getpid(&ppid);
   return ppid;
}
#endif

/********************** Function getuid ************************************/

uid_t getuid (void)
{
   int euid;
   return _getuid(&euid);
}

/********************** Function geteuid ************************************/

int geteuid()
{
   int euid;
   _getuid(&euid);
   return euid;
}

/********************** Function getgid ************************************/

#ifdef L_getgid
int getgid()
{
   int egid;
   return __getgid(&egid);
}
#endif

/********************** Function getegid ************************************/

int getegid()
{
   int egid;
   _getgid(&egid);
   return egid;
}

/********************** Function dup2 ************************************/

#ifdef L_dup2

#include <fcntl.h>

int dup2(ifd, ofd)
int ifd;
{
   return fcntl(ifd, F_DUPFD, ofd);
}
#endif

/********************** Function dup ************************************/

#ifdef L_dup
#include <sys/param.h>
#include <fcntl.h>
#include <errno.h>

/* This is horribly complicated, there _must_ be a better way! */

int
dup(fd)
int fd;
{
   int nfd;
   extern int errno;
   int oerr = errno;

   errno = 0;
   for(nfd=0; nfd<NR_OPEN; nfd++)
   {
      if( fcntl(nfd, F_GETFD) < 0 )
         break;
   }
   if( nfd == NR_OPEN ) { errno = EMFILE ; return -1; }
   errno = oerr;
   if( fcntl(fd, F_DUPFD, nfd) < 0 )
   {
      if( errno == EINVAL ) errno = EMFILE;
      return -1;
   }
   return nfd;
}
#endif

/********************** Function getpgrp ************************************/

#ifdef L_getpgrp
int
getpgrp()
{
   return getpgid(0);
}
#endif

/********************** Function times ************************************/

#ifdef L_times
clock_t times(buf)
struct tms* buf;
{
   long rv;
   __times(buf, &rv);
   return rv;
}
#endif

//------------------------------------------------------------------------------

time_t time (time_t *where)
{
   struct timeval rv;
   if( gettimeofday(&rv, (void*)0) < 0 ) return -1;
   if(where) *where = rv.tv_sec;
   return rv.tv_sec;
}

//------------------------------------------------------------------------------

void abort ()
{
   signal (SIGABRT, SIG_DFL);
   kill (SIGABRT, getpid ());  // first try
   pause ();  // system may just schedule
   signal (SIGKILL, SIG_DFL);
   kill (SIGKILL, getpid ());  // second try
   _exit (255);  // third try
}

/********************** Function wait ************************************/

int wait(int * status)
{
   return wait4(-1, status, 0, (void*)0);
}

/********************** Function wait3 **************************************/

#ifdef L_wait3
int
wait3(status, opts, usage)
int * status;
int opts;
struct rusage * usage;
{
   return wait4(-1, status, opts, usage);
}
#endif

/********************** Function waitpid ************************************/

int
waitpid(pid, status, opts)
int pid;
int * status;
int opts;
{
   return wait4(pid, status, opts, (void*)0);
}

/********************** Function killpg ************************************/

#ifdef L_killpg
int
killpg(pid, sig)
int pid;
int sig;
{
   if(pid == 0)
       pid = getpgrp();
   if(pid > 1)
       return kill(-pid, sig);
   errno = EINVAL;
   return -1;
}
#endif

/********************** Function setpgrp ************************************/

#ifdef L_setpgrp
int
setpgrp()
{
   return setpgid(0,0);
}
#endif

/********************** Function sleep ************************************/

#ifdef L_sleep

#ifdef __ELKS__
/* This uses SIGALRM, it does keep the previous alarm call but will lose
 * any alarms that go off during the sleep
 */

static void alrm() { }

unsigned int sleep(seconds)
unsigned int seconds;
{
   void (*last_alarm)();
   unsigned int prev_sec;

   prev_sec = alarm(0);
   if( prev_sec <= seconds ) prev_sec = 1; else prev_sec -= seconds;

   last_alarm = signal(SIGALRM, alrm);
   alarm(seconds);
   pause();
   seconds = alarm(prev_sec);
   signal(SIGALRM, last_alarm);
   return seconds;
}

#else
        /* Is this a better way ? If we have select of course :-) */
unsigned int
sleep(seconds)
unsigned int seconds;
{
        struct timeval timeout;
	time_t start = time((void*)0);
        timeout.tv_sec = seconds;
        timeout.tv_usec = 0;
        select(1, NULL, NULL, NULL, &timeout);
	return seconds - (time((void*)0) - start);
}
#endif

#endif

/********************** Function usleep ************************************/

#ifdef L_usleep
void
usleep(useconds)
unsigned long useconds;
{
        struct timeval timeout;
        timeout.tv_sec = useconds%1000000L;
        timeout.tv_usec = useconds/1000000L;
        select(1, NULL, NULL, NULL, &timeout);
}
#endif

/********************** Function mkfifo ************************************/

#ifdef L_mkfifo
int
mkfifo(path, mode)
char * path;
int mode;
{
   return mknod(path, mode | S_IFIFO, 0);
}
#endif

/********************** THE END ********************************************/
