#ifdef L_getppid
#include <unistd.h>

int
getppid(void)
{
   int ppid;
   __getpid(&ppid);
   return ppid;
}
#endif
