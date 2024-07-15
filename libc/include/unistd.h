#ifndef __UNISTD_H
#define __UNISTD_H

#include <features.h>
#include <sys/types.h>
#include <sys/select.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

ssize_t read(int __fd, void * __buf, size_t __nbytes);
ssize_t write(int __fd, const void * __buf, size_t __n);
int     pipe(int __pipedes[2]);
char*   crypt(const char *__key, const char *__salt);

unsigned int alarm(unsigned int seconds);
unsigned int sleep(unsigned int seconds);
int          usleep(unsigned long useconds);

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#ifndef R_OK
#define	R_OK	4		/* Test for read permission.  */
#define	W_OK	2		/* Test for write permission.  */
#define	X_OK	1		/* Test for execute permission.  */
#define	F_OK	0		/* Test for existence.  */
#endif

#define _POSIX_VDISABLE	'\0'

// Functions

int brk(const void *addr);
int _brk (unsigned int newbrk);                 /* syscall */
void *sbrk(int increment);
int _sbrk(int increment, void **pnewbrk);       /* syscall */

int close (int fildes);
int dup (int fildes);
int dup2 (int oldfd, int newfd);
int execl(const char *fname, const char *arg0, ... /*, (char *)0 */);
int execle(const char *fname, const char *arg0, ... /*, (char *)0, char *envp[] */);
int execlp(const char *fname, const char *arg0, ... /*, (char *)0 */);
int execlpe(const char *fname, const char *arg0, ... /*, (char *)0, char *envp[] */);
int execv(const char *fname, char **argv);
int execve(const char *fname, char **argv, char **envp);
int execvp(const char *fname, char **argv);
int execvpe(const char *fname, char **argv, char **envp);
int _execve(const char *fname, char *stk_ptr, int stack_bytes); /* syscall */
int isatty (int fd);
char *ttyname(int fd);
off_t lseek (int fildes, off_t offset, int whence);
int _lseek (int fd, off_t *posn, int where);    /* syscall */
int link(const char *path1, const char *path2);
int symlink(const char *path1, const char *path2);
int unlink(const char *fname);
int access(const char *path, int mode);
ssize_t readlink(const char * restrict path, char * restrict buf, size_t bufsize);
int chdir(const char *path);
int rmdir(const char *path);

int chown(const char *path, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);

pid_t fork(void);
pid_t vfork(void);
pid_t setsid(void);
pid_t getpid(void);
pid_t getppid(void);
uid_t _getpid(int *ppid);

/*int setpgid(pid_t pid,pid_t pgid);*/  /* NYI */
/*int getpgid(pid_t pid);*/             /* NYI */
/*int getpgrp(void);*/                  /* NYI */
/*int setpgrp(void);*/                  /* not compiled */
/*int getsid(void);*/                   /* NYI */

uid_t getuid (void);
int setuid(uid_t uid);
int setgid(uid_t gid);
uid_t _getuid(int *euid);
uid_t getgid(void);
uid_t _getgid(int *egid);
uid_t getegid(void);
uid_t geteuid(void);

char * getcwd (char * buf, size_t size);
void sync(void);

int getopt(int argc, char * const argv[], const char *opts);
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;

extern int     __argc;
extern char ** __argv;
extern char *  __program_filename;          /* process argv[0] */
extern char ** environ;                     /* process global environment */

#endif
