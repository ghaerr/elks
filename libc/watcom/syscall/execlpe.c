#include <unistd.h>

int
execlpe(const char *fname, const char *arg0, ... /*, (char *)0, char *envp[] */)
{
    char **envp = (char **)&arg0;

    while(*envp) envp++;
    return execvpe(fname, (char **)&arg0, envp+1);
}
