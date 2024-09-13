#include <unistd.h>

int
execl(const char *fname, const char *arg0, ... /*, (char *)0 */)
{
    return execve(fname, (char **)&arg0, environ);
}
