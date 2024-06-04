/* Copyright (C) 1995,1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */
#include <string.h>
#include <sys/types.h>

#ifdef L_abs
int
abs(arg1)
int arg1;
{
   return arg1>0?arg1:-arg1;
}
#endif

#ifdef L_labs
long
labs(arg1)
long arg1;
{
   return arg1>0?arg1:-arg1;
}
#endif

#ifdef L_raise
int
raise(signo)
int signo;
{
   return kill(getpid(), signo);
}
#endif

#ifdef L_bcopy
#undef bcopy
void
bcopy(src, dest, len)
const void * src;
void *dest;
int len;
{
   (void) memcpy(dest, src, len);
}
#endif

#ifdef L_bzero
#undef bzero
void
bzero(dest, len)
void *dest;
int len;
{
   (void) memset(dest, '\0', len);
}
#endif

#ifdef L_bcmp
#undef bcmp
int
bcmp(dest, src, len)
const void * src, *dest;
int len;
{
   return memcmp(dest, src, len);
}
#endif

#ifdef L_index
#undef index
char *
index(src, chr)
const char *src;
int chr;
{
   return strchr(src, chr);
}
#endif

#ifdef L_rindex
#undef rindex
char *
rindex(src, chr)
const char *src;
int chr;
{
   return strrchr(src, chr);
}
#endif

#ifdef L_remove
#include <errno.h>

int
remove(src)
const char *src;
{
   extern int errno;
   int er = errno;
   int rv = unlink(src);
   if( rv < 0 && errno == EISDIR )
      rv = rmdir(src);
   if( rv >= 0 ) errno = er;
   return rv;
}
#endif

#include <fcntl.h>

int
creat(const char *file, mode_t mode)
{
   return open(file, O_TRUNC|O_CREAT|O_WRONLY, mode);
}
