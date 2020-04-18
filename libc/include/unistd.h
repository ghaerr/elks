
#pragma once

#include <features.h>
#include <sys/types.h>
#include <sys/select.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

extern ssize_t read __P ((int __fd, void * __buf, size_t __nbytes));
extern ssize_t write __P ((int __fd, __const void * __buf, size_t __n));
extern int pipe __P ((int __pipedes[2]));
extern unsigned int alarm __P ((unsigned int __seconds));
extern unsigned int sleep __P ((unsigned int __seconds));
extern int pause __P ((void));
extern char*    crypt __P((__const char *__key, __const char *__salt));

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

int brk (void * addr);
void * sbrk (intptr_t increment);

int close (int fildes);
int dup (int fildes);
int dup2 (int oldfd, int newfd);
int execl(char *fname, char *arg0, ...);
int execle(char *fname, char *arg0, ...);
int execlp(char *fname, char *arg0, ...);
int execv(char *fname, char **argv);
int execve(char *fname, char **argv, char **envp);
int _execve(char *fname, char *stk_ptr, int stack_bytes);
int execvp(char *fname, char **argv);
void _exit(int status);
int isatty (int fd);
char *ttyname(int fd);
off_t lseek (int fildes, off_t offset, int whence);
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
int getpgrp(void);
int setpgrp(void);

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
void usleep(unsigned long useconds);

extern char ** environ;
