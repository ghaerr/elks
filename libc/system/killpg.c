#ifdef L_killpg
#include <signal.h>

int
killpg(int pid, int sig)
{
   if(pid == 0)
       pid = getpgrp();
   if(pid > 1)
       return kill(-pid, sig);
   errno = EINVAL;
   return -1;
}
#endif
