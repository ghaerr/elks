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
    if (fork() == 0) {
        for (;;) {
            status &= ~ABORT;
            kill(getpid(), SIGABRT);
            if (!(status & ABORT)) printf("ABORT failed\n");
            if (getppid() == 1) exit(1);
        }
    }
    for(;;) {
        status &= ~INT;
        kill(getpid(), SIGINT);
        if (!(status & INT)) printf("INT failed\n");
    }
    return 0;
}
