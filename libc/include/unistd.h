
#pragma once

#include <features.h>
#include <sys/types.h>

typedef int intptr_t;
typedef intptr_t ssize_t;

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

extern ssize_t read __P ((int __fd, char * __buf, size_t __nbytes));
extern ssize_t write __P ((int __fd, __const char * __buf, size_t __n));
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
int dup2 (int oldfd, int newfd);
int isatty (int fd);
off_t lseek (int fildes, off_t offset, int whence);

pid_t fork ();
pid_t getpid ();
pid_t setsid ();

uid_t getuid (void);

char * getcwd (char * buf, size_t size);
