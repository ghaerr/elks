#include <unistd.h>

int
execvp(const char *fname, char **argv)
{
   return execvpe(fname, argv, environ);
}
