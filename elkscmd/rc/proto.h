/* proto.h
	This file provides a definition for size_t and align_t that
	should work for your system. If it does not, it is up to you to
	make it the right thing. The problem is that I cannot rely upon
	<sys/params.h> to do the right thing on machines which don't
	yet have ansi header files. Note that on many RISC machines,
	align_t must be at least 32 bits wide, and sparc doubles are
	aligned on 64 bit boundaries, but of course rc does not use
	doubles in its code, so the "typedef long ALIGN_T" is good
	enough in the sparc's case. Also for performance reasons on a
	VAX one would probably want align_t to be 32 bits wide.

	You can override these definitions with compile-line definitions
	of the same macros.
*/

#ifndef ALIGN_T
typedef long ALIGN_T;
#endif
#ifndef SIZE_T
typedef unsigned int SIZE_T;
#endif
#ifndef MODE_T
typedef short int MODE_T;
#endif
#ifndef PID_T
typedef int PID_T;
#endif
#ifndef SIG_ATOMIC_T
typedef int SIG_ATOMIC_T;
#endif

/* fake stdlib.h */

extern void exit(int);
extern void qsort(void *, SIZE_T, SIZE_T, int (*)(const void *, const void *));

/* fake string.h */

extern int strncmp(const char *, const char *, SIZE_T);
extern int strcmp(const char *, const char *);
extern SIZE_T strlen(const char *);
extern char *strchr(const char *, int);
extern char *strrchr(const char *, int);
extern char *strcpy(char *, const char *);
extern char *strncpy(char *, const char *, SIZE_T);
extern char *strcat(char *, const char *);
extern char *strncat(char *, const char *, SIZE_T);
extern void *memcpy(void *, const void *, SIZE_T);
extern void *memset(void *, int, SIZE_T);

/* fake unistd.h */

extern PID_T fork(void);
extern PID_T getpid(void);
extern char *getenv(const char *);
extern int chdir(const char *);
extern int close(int);
extern int dup(int);
extern int dup2(int, int);
extern int execve(const char *, const char **, const char **);
extern int execl(const char *,...);
extern int getegid(void);
extern int geteuid(void);
extern int getgroups(int, int *);
/*extern int ioctl(int, long,...);*/ /* too much trouble leaving this uncommented */
extern int isatty(int);
#ifndef SYSVR4 /* declares AND defines this in sys/stat.h!! */
extern int mknod(const char *, int, int);
#endif
extern int pipe(int *);
extern int read(int, void *, unsigned int);
extern int setpgrp(int, PID_T);
extern int unlink(const char *);
extern int wait(int *);
extern int write(int, const void *, unsigned int);

/* fake errno.h for mips (which doesn't declare errno in errno.h!?!?) */

#ifdef host_mips
extern int errno;
#endif
