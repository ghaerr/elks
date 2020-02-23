#ifdef L_dup2

#include <fcntl.h>
#include <unistd.h>

int
dup2(int ifd, int ofd)
{
   return fcntl(ifd, F_DUPFD, ofd);
}
#endif
