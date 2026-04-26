#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static volatile sig_atomic_t got_alarm;

static void alarm_handler(int sig)
{
    (void)sig;
    signal(SIGALRM, alarm_handler);
    got_alarm = 1;
}

static int wait_any_with_alarm(int *status, int seconds)
{
    int rc;

    got_alarm = 0;
    signal(SIGALRM, alarm_handler);
    errno = 0;
    alarm(seconds);
    rc = waitpid((pid_t)-1, status, 0);
    alarm(0);
    return rc;
}

static void finish_by_execing_shell(void)
{
    char *argv_sh[] = { "sh", (char *)0 };
    char *argv_sash[] = { "sash", (char *)0 };

    printf("[orphan-zombie-pid1] trying to exec a shell\n");
    execv("/bin/sh", argv_sh);
    execv("/bin/sash", argv_sash);

    printf("[orphan-zombie-pid1] no shell found, sleeping forever\n");
    for (;;)
        sleep(60);
}

int main(void)
{
    pid_t middle;
    pid_t r1, r2;
    int st1 = 0, st2 = 0;
    int saw_24 = 0, saw_42 = 0;

    printf("[orphan-zombie-pid1] starting\n");

    if (getpid() != 1) {
        printf("SKIP: this test must run as pid 1 (temporary /bin/init)\n");
        return 77;
    }

    middle = fork();
    if (middle < 0) {
        perror("fork(middle)");
        return 2;
    }

    if (middle == 0) {
        pid_t leaf = fork();
        if (leaf < 0)
            _exit(100);
        if (leaf == 0)
            _exit(42);

        /* Exit without waiting: the grandchild is now an orphaned zombie case. */
        _exit(24);
    }

    r1 = waitpid((pid_t)-1, &st1, 0);
    if (r1 < 0) {
        perror("waitpid(first)");
        return 2;
    }

    if (WIFEXITED(st1)) {
        if (WEXITSTATUS(st1) == 24)
            saw_24 = 1;
        if (WEXITSTATUS(st1) == 42)
            saw_42 = 1;
    }

    errno = 0;
    r2 = wait_any_with_alarm(&st2, 2);
    if (r2 < 0) {
        if (errno == ECHILD) {
            printf("FAIL: second wait returned -ECHILD; adopted zombie was lost\n");
            finish_by_execing_shell();
        }
        if (errno == EINTR && got_alarm) {
            printf("FAIL: second wait blocked until SIGALRM; adopted zombie was not waitable\n");
            finish_by_execing_shell();
        }
        perror("waitpid(second)");
        finish_by_execing_shell();
    }

    if (WIFEXITED(st2)) {
        if (WEXITSTATUS(st2) == 24)
            saw_24 = 1;
        if (WEXITSTATUS(st2) == 42)
            saw_42 = 1;
    }

    if (saw_24 && saw_42)
        printf("[orphan-zombie-pid1] PASS\n");
    else
        printf("[orphan-zombie-pid1] FAIL: statuses seen were 0x%04x and 0x%04x\n", st1, st2);

    finish_by_execing_shell();
    return 0;
}
