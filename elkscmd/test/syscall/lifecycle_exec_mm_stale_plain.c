#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define INNER_FORKS 16

int main(void)
{
    int i;
    int status;

    for (i = 0; i < INNER_FORKS; i++) {
        pid_t child = fork();
        if (child < 0) {
            perror("plain: fork");
            return 2;
        }
        if (child == 0)
            _exit(0);
        if (waitpid(child, &status, 0) != child) {
            perror("plain: waitpid");
            return 2;
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            printf("plain: unexpected child status 0x%04x\n", status);
            return 1;
        }
    }

    return 0;
}
