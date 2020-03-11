
#include <stdio.h>


FILE * popen(command, rw)
char * command;
char * rw;
{
   int pipe_fd[2];
   int pid, reading;

   if( pipe(pipe_fd) < 0 ) return NULL;
   reading = (rw[0] == 'r');

   pid = vfork();
   if( pid < 0 ) { close(pipe_fd[0]); close(pipe_fd[1]); return NULL; }
   if( pid == 0 )
   {
      close(pipe_fd[!reading]);
      close(reading);
      if( pipe_fd[reading] != reading )
      {
	 dup2(pipe_fd[reading], reading);
         close(pipe_fd[reading]);
      }

      execl("/bin/sh", "sh", "-c", command, (char*)0);
      _exit(255);
   }

   close(pipe_fd[reading]);
   return fdopen(pipe_fd[!reading], rw);
}

int pclose(fd)
FILE *fd;
{
   int waitstat;
   if( fclose(fd) != 0 ) return EOF;
   wait(&waitstat);
}

