#include <signal.h>

/* NOTE: sigaction only partially implemented */
int sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
     if (signal(sig, act->sa_handler) == (sighandler_t)-1)
        return -1;
     return 0;
}

int sigemptyset(sigset_t *set)
{
    *set = 0;
    return 0;
}
