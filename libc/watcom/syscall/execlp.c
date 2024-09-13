#include <unistd.h>

int
execlp(const char *fname, const char *arg0, ... /*, (char *)0 */)
{
    return execvp(fname, (char **)&arg0);
}
