#ifdef L_wait3
#include <sys/wait.h>

int
wait3(int * status, int opts, struct rusage * usage)
{
   return wait4(-1, status, opts, usage);
}
#endif
