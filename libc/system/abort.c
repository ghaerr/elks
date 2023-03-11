#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void
abort(void)
{
    /* call registered handler if any, or die if none */
    kill (getpid(), SIGABRT);

    /* no registered handler, die on SIGABRT */
    signal (SIGABRT, SIG_DFL);
    kill (getpid(), SIGABRT);

    /* should not be needed */
    _exit(255);
}
