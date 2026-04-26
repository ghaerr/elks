#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define OUTER_ITERS 200

int main(int argc, char **argv)
{
    const char *multiseg = (argc > 1)? argv[1]: "/bin/lifecycle_exec_mm_stale_multiseg";
    const char *plain = (argc > 2)? argv[2]: "/bin/lifecycle_exec_mm_stale_plain";
    int i;
    int status;

    printf("[exec-mm-stale] starting with multiseg='%s' plain='%s'\n", multiseg, plain);

    for (i = 0; i < OUTER_ITERS; i++) {
        pid_t child = fork();
        if (child < 0) {
            perror("runner: fork");
            return 2;
        }
        if (child == 0) {
            char *av[3];
            av[0] = (char *)multiseg;
            av[1] = (char *)plain;
            av[2] = (char *)0;
            execv(multiseg, av);
            perror("runner: execv(multiseg)");
            _exit(111);
        }
        if (waitpid(child, &status, 0) != child) {
            perror("runner: waitpid");
            return 2;
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            printf("[exec-mm-stale] FAIL at iter=%d status=0x%04x\n", i, status);
            return 1;
        }
    }

    printf("[exec-mm-stale] PASS (stress completed)\n");
    return 0;
}
