#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <paths.h>

int
system(const char *command)
{
   int status, pid;
   sighandler_t save_quit;
   sighandler_t save_int;

   if( command == 0 ) return 1;

   save_quit = signal(SIGQUIT, SIG_IGN);
   save_int  = signal(SIGINT,  SIG_IGN);
   if( (pid=vfork()) < 0 )
   {
      signal(SIGQUIT, save_quit);
      signal(SIGINT,  save_int);
      return -1;
   }
   if( pid == 0 )
   {
      signal(SIGQUIT, SIG_DFL);
      signal(SIGINT,  SIG_DFL);

      execl(_PATH_BSHELL, "sh", "-c", command, (char*)0);
      _exit(127);
   }

   /* Signals are not absolutely guarenteed with vfork */
   signal(SIGQUIT, SIG_IGN);
   signal(SIGINT,  SIG_IGN);

   /* wait for child termination*/
   while (waitpid(pid, &status, 0) != pid)
		continue;

   signal(SIGQUIT, save_quit);
   signal(SIGINT,  save_int);
   return status;
}
