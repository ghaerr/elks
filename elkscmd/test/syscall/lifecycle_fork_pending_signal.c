#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FORK_ITERS   250
#define SIGNAL_ITERS 5000
#define SIGNAL_USEC  1000UL
#define BALLAST_SIZE 4096

static int alert_pipe[2] = { -1, -1 };
static pid_t parent_pid;
static char ballast[BALLAST_SIZE];

static void sigusr1_handler(int sig)
{
    pid_t me;

    (void)sig;
    signal(SIGUSR1, sigusr1_handler);     /* ELKS signal handlers are one-shot */

    me = getpid();
    if (me != parent_pid && alert_pipe[1] >= 0)
        (void)write(alert_pipe[1], (char *)&me, sizeof(me));
}

static int waitpid_retry(pid_t pid, int *status, int options)
{
    int rc;

    do {
        errno = 0;
        rc = waitpid(pid, status, options);
    } while (rc < 0 && errno == EINTR);

    return rc;
}

int main(void)
{
    pid_t sender;
    int i;
    int status;
    int bad = 0;
    pid_t offender;
    int n;

    printf("[fork-pending-signal] starting\n");

    for (i = 0; i < BALLAST_SIZE; i++)
        ballast[i] = (char)(i ^ 0x5a);

    parent_pid = getpid();

    if (pipe(alert_pipe) < 0) {
        perror("pipe");
        return 2;
    }

    signal(SIGUSR1, sigusr1_handler);

    sender = fork();
    if (sender < 0) {
        perror("fork(sender)");
        return 2;
    }

    if (sender == 0) {
        for (i = 0; i < SIGNAL_ITERS; i++) {
            if (kill(parent_pid, SIGUSR1) < 0)
                break;
            usleep(SIGNAL_USEC);
        }
        _exit(0);
    }

    for (i = 0; i < FORK_ITERS; i++) {
        pid_t child = fork();
        if (child < 0) {
            perror("fork(worker)");
            kill(sender, SIGKILL);
            return 2;
        }
        if (child == 0)
            _exit(0);

        if (waitpid_retry(child, &status, 0) != child) {
            perror("waitpid(worker)");
            kill(sender, SIGKILL);
            return 2;
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            printf("FAIL: worker child exited unexpectedly: status=0x%04x\n", status);
            kill(sender, SIGKILL);
            return 1;
        }
    }

    if (waitpid_retry(sender, &status, 0) != sender) {
        perror("waitpid(sender)");
        return 2;
    }

    close(alert_pipe[1]);
    alert_pipe[1] = -1;

    while ((n = read(alert_pipe[0], (char *)&offender, sizeof(offender))) == sizeof(offender)) {
        if (!bad)
            printf("FAIL: child %d executed parent-directed SIGUSR1 handler\n", offender);
        bad++;
    }

    if (bad) {
        printf("[fork-pending-signal] FAIL (%d offending child event%s)\n",
            bad, bad == 1 ? "" : "s");
        return 1;
    }

    printf("[fork-pending-signal] PASS\n");
    return 0;
}
