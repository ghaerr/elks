#ifndef __SIGNAL_H
#define __SIGNAL_H

#include <features.h>
#include <sys/types.h>

#include __SYSINC__(signal.h)

sighandler_t signal(int number, sighandler_t pointer);
int kill (pid_t pid, int sig);
int killpg (int pid, int sig);

#endif
