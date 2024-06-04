#ifndef __SIGNAL_H
#define __SIGNAL_H

#include <features.h>
#include <sys/types.h>

#include __SYSINC__(signal.h)

sighandler_t signal(int number, sighandler_t pointer);
int _signal(int __sig, __kern_sighandler_t __cbfunc);   /* syscall */

typedef struct siginfo_t siginfo_t;

union __sigaction_u {
     void    (*__sa_handler)(int);
     void    (*__sa_sigaction)(int, siginfo_t *, void *);
};

struct  sigaction {
    union __sigaction_u __sigaction_u;  /* signal handler */
    sigset_t sa_mask;                   /* signal mask to apply */
    int sa_flags;                       /* see signal options below */
 };

#define sa_handler      __sigaction_u.__sa_handler
#define sa_sigaction    __sigaction_u.__sa_sigaction

/* NOTE: sigaction only partially implemented */
int sigaction(int sig, const struct sigaction *act, struct sigaction *oact);
int sigemptyset(sigset_t *set);

int kill (pid_t pid, int sig);
int killpg (int pid, int sig);

#endif
