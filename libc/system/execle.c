#include <unistd.h>

int
execle(char *fname, char *arg0, ...)
{
   char ** envp = &arg0;
   while(*envp) envp++;
   return execve(fname, &arg0, envp+1);
}
