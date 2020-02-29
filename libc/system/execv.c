#include <unistd.h>

int
execv(char *fname, char **argv)
{
   return execve(fname, argv, environ);
}
