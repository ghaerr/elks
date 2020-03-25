#include <unistd.h>

pid_t
getppid(void)
{
   int ppid;
   _getpid(&ppid);
   return ppid;
}
