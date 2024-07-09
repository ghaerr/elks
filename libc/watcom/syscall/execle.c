#include <unistd.h>

int
execle(const char *fname, const char *arg0, ... /*, (char *)0, char *envp[] */)
{
    char **envp = (char **)&arg0;

    while(*envp) envp++;
    return execve(fname, (char **)&arg0, envp+1);
}
