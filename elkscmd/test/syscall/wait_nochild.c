#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static volatile int got_alarm;

static void alarm_handler(int sig)
{
    (void)sig;
    signal(SIGALRM, alarm_handler);
    got_alarm = 1;
}

static int expect_echild_wnohang(pid_t pid, const char *label)
{
    int status = 0;
    int rc;
    int saved_errno;

    errno = 0;
    rc = waitpid(pid, &status, WNOHANG);
    saved_errno = errno;

    if (rc == -1 && saved_errno == ECHILD) {
        printf("PASS: %s -> -1/ECHILD\n", label);
        return 0;
    }

    printf("FAIL: %s -> rc=%d errno=%d status=0x%04x (expected -1/ECHILD)\n",
        label, rc, saved_errno, status);
    return 1;
}

static int cleanup_child(int child, int write_fd)
{
    int status;
    int rc;

    close(write_fd);
    errno = 0;
    rc = waitpid(child, &status, 0);
    if (rc != child) {
        printf("FAIL: cleanup waitpid(child=%d) -> rc=%d errno=%d status=0x%04x\n",
            child, rc, errno, status);
        return 1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        printf("FAIL: cleanup child exited abnormally status=0x%04x\n", status);
        return 1;
    }

    return 0;
}

static int expect_echild_blocking_with_live_nonmatching_child(void)
{
    int pipefd[2];
    int child;
    pid_t nonmatch;
    int status = 0;
    int rc;
    int saved_errno;
    int fails = 0;
    char ch;

    if (pipe(pipefd) < 0) {
        perror("pipe");
        return 1;
    }

    child = fork();
    if (child < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }

    if (child == 0) {
        signal(SIGALRM, SIG_DFL);
        close(pipefd[1]);
        while (read(pipefd[0], &ch, 1) < 0) {
            if (errno != EINTR)
                _exit(2);
        }
        close(pipefd[0]);
        _exit(0);
    }

    close(pipefd[0]);

    nonmatch = child + 1;
    if (nonmatch <= 0)
        nonmatch = child + 2;

    got_alarm = 0;
    signal(SIGALRM, alarm_handler);
    errno = 0;
    alarm(2);
    rc = waitpid(nonmatch, &status, 0);
    saved_errno = errno;
    alarm(0);

    if (rc == -1 && saved_errno == ECHILD && !got_alarm) {
        printf("PASS: waitpid(non-child-pid=%d, ..., 0) with live child %d -> -1/ECHILD immediately\n",
            nonmatch, child);
    } else if (rc == -1 && saved_errno == EINTR && got_alarm) {
        printf("FAIL: waitpid(non-child-pid=%d, ..., 0) with live child %d slept until SIGALRM\n",
            nonmatch, child);
        fails++;
    } else {
        printf("FAIL: waitpid(non-child-pid=%d, ..., 0) with live child %d -> rc=%d errno=%d status=0x%04x alarm=%d\n",
            nonmatch, child, rc, saved_errno, status, (int)got_alarm);
        fails++;
    }

    fails += cleanup_child(child, pipefd[1]);
    return fails;
}

int main(void)
{
    int fails = 0;
    pid_t impossible_pid = 32767;

    printf("[wait-nochild] starting\n");

    fails += expect_echild_wnohang((pid_t)-1,
        "waitpid(-1, ..., WNOHANG) with no children");
    fails += expect_echild_wnohang(impossible_pid,
        "waitpid(non-child-pid, ..., WNOHANG) with no children");
    fails += expect_echild_blocking_with_live_nonmatching_child();

    if (fails) {
        printf("[wait-nochild] FAIL (%d subtest%s failed)\n",
            fails, fails == 1 ? "" : "s");
        return 1;
    }

    printf("[wait-nochild] PASS\n");
    return 0;
}
