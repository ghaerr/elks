#include <unistd.h>

int
execvp(char *fname, char **argv)
{
   return execvpe(fname, argv, environ);
}
