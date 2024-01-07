#ifdef L_dup
#include <sys/param.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

/* This is horribly complicated, there _must_ be a better way! */

int
dup(int fd)
{
   int nfd;
   int oerr = errno;

   for(nfd=0; nfd<NR_OPEN; nfd++)
   {
      if( fcntl(nfd, F_GETFD) < 0 )
         break;
   }
   if( nfd == NR_OPEN ) { errno = EMFILE ; return -1; }
   errno = oerr;
   if( fcntl(fd, F_DUPFD, nfd) < 0 )
   {
      if( errno == EINVAL ) errno = EMFILE;
      return -1;
   }
   return nfd;
}
#endif
