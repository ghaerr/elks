#ifndef __SIGNAL_H
#define __SIGNAL_H

#include <sys/types.h>

#undef __KERNEL__
#include __SYSINC__(signal.h)

/* BSDisms */
#ifdef BSD
extern const char * const sys_siglist[];
#define sig_t sighandler_t
#endif

sighandler_t signal(int number, sighandler_t pointer);
int kill (pid_t pid, int sig);
int killpg (int pid, int sig);

#endif
