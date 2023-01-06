#include <sys/types.h>
#include <sys/wait.h>

#ifdef L_wait3
pid_t
wait3(int * status, int opts, struct rusage * usage)
{
   return wait4(-1, status, opts, usage); /* FIXME kernel ignores 'usage' */
}
#endif
