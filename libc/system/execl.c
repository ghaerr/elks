#include <unistd.h>

int
execl(char *fname, char *arg0, ...)
{
   return execve(fname, &arg0, environ);
}
