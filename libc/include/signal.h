#ifndef __SIGNAL_H
#define __SIGNAL_H

#include <sys/types.h>
#include <features.h>

#undef __KERNEL__
#include __SYSINC__(signal.h)

/* BSDisms */
#ifdef BSD
extern __const char * __const sys_siglist[];
#define sig_t __sighandler_t
#endif

__sighandler_t signal(int number, __sighandler_t pointer);
int kill (pid_t pid, int sig);
int killpg (int pid, int sig);

#endif
