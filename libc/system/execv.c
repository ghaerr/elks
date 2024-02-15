#include <unistd.h>

int
execv(const char *fname, char **argv)
{
   return execve(fname, argv, environ);
}
