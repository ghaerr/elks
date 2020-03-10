#ifndef __SIGNAL_H
#define __SIGNAL_H

#include <types.h>
#include <features.h>

#undef __KERNEL__
#include __SYSINC__(signal.h)

/* Type of a signal handler to pass to the kernel.  This is always a `stdcall'
   function, even if the user program is being compiled for a different
   calling convention.  */
typedef void (__attribute__((stdcall)) *__kern_sighandler_t)(int);

/* BSDisms */
#ifdef BSD
extern __const char * __const sys_siglist[];
#define sig_t __sighandler_t
#endif

__sighandler_t signal(int number, __sighandler_t pointer);
int kill (pid_t pid, int sig);
int killpg (int pid, int sig);

#endif
