/*
 * Confirm signal loss from kernel clearing signal mask
 * before calling any registered signal handler.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define ABORT   1
#define INT     2
volatile int status;

static int pid;

void sigabort(int signo)
{
    status |= ABORT;
    signal(SIGABRT, sigabort);  // must reregister handler each time
}

void sigint(int signo)
{
    status |= INT;
    signal(SIGINT, sigint);     // must reregister handler each time
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigint);
    signal(SIGABRT, sigabort);
    pid = getpid();
    if (fork() == 0) {
        signal(SIGINT, SIG_IGN);
        signal(SIGABRT, SIG_IGN);
        for (;;) {
            status &= ~ABORT;
            kill(pid, SIGABRT);
            if (!(status & ABORT)) printf("ABORT failed\n");
            if (getppid() == 1) exit(1);
        }
    }
    for(;;) {
        status &= ~INT;
        kill(pid, SIGINT);
        if (!(status & INT)) printf("INT failed\n");
        if (getppid() == 1) exit(1);
    }
    return 0;
}
