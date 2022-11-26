#include <unistd.h>

int
execvp(char *fname, char **argv)
{
   return _execvpe(fname, argv, environ);
}
