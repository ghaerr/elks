#ifdef L_execlp
#include <unistd.h>

int
execlp(char *fname, char *arg0)
{
   return execvp(fname, &arg0);
}
#endif
