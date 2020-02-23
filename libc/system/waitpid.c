#include <sys/types.h>
#include <sys/wait.h>

pid_t
waitpid(pid_t pid, int * status, int opts)
{
   return wait4(pid, status, opts, (void *)0);
}
