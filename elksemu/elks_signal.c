#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include "elks.h"

static int elks_sigtrap_ip = -1, elks_sigtrap_cs = -1;

void
sig_trap(int signo)
{
#if USE_PTRACE
    pid_t child = elks_cpu.child, pid;
    int status;
    kill(elks_cpu.child, SIGSTOP);
    while (ptrace(PTRACE_GETREGS, child, NULL, &elks_cpu.regs) != 0) {
        if (errno != ESRCH)
            return;
    }
#endif
    elks_cpu.xsp -= 2;
    ELKS_POKE(unsigned short, elks_cpu.xsp, signo);
    elks_cpu.xsp -= 2;
    ELKS_POKE(unsigned short, elks_cpu.xsp, elks_cpu.xcs);
    elks_cpu.xsp -= 2;
    ELKS_POKE(unsigned short, elks_cpu.xsp, elks_cpu.xip);
    elks_cpu.xip = elks_sigtrap_ip;
    elks_cpu.xcs = elks_sigtrap_cs;
#if USE_PTRACE
    if (ptrace(PTRACE_SETREGS, child, NULL, &elks_cpu.regs) != 0)
        return;
    kill(elks_cpu.child, SIGCONT);
#endif
}

int
elks_signal(int bx, int cx, int dx, int di, int si)
{
    void (*oldsig) (int)= 0;
    if (bx < 0 || bx >= NSIG) {
        errno = EINVAL;
        return -1;
    }
    if (cx == 0 && dx == 0)
        oldsig = signal(bx, SIG_DFL);
    else if (cx == 1 && dx == 0)
        oldsig = signal(bx, SIG_IGN);
    else {
        elks_sigtrap_ip = cx;
        elks_sigtrap_cs = dx;
        oldsig = signal(bx, sig_trap);
    }
    if (oldsig == SIG_ERR)
        return -1;
    if (oldsig == SIG_DFL)
        return 0;
    if (oldsig == SIG_IGN)
        return 1;
    return 2;
}
