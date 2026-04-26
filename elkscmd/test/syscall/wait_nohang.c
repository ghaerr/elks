#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>

int main(void)
{
    int st = 0;
    int r;

    errno = 0;
    r = waitpid(-1, &st, WNOHANG);

    if (r == 0)
        printf("BUG: returned 0 with no children\n");
    else if (r == -1 && errno == ECHILD)
        printf("OK: returned ECHILD with no children\n");
    else
        printf("Unexpected: r=%d errno=%d\n", r, errno);

    return 0;
}
