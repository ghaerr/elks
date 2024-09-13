#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/rtinit.h>

FILE *__IO_list = 0;

#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
DESTRUCTOR(__stdio_fini, _INIT_PRI_STDIO);
void __stdio_fini(void)
{
   FILE *fp;

   fflush(stdout);
   fflush(stderr);
   for (fp = __IO_list; fp; fp = fp->next)
   {
      fflush(fp);
      close(fp->fd);
      /* Note we're not de-allocating the memory */
      /* There doesn't seem to be much point :-) */
      fp->fd = -1;
   }
}
#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
CONSTRUCTOR(__stdio_init, _INIT_PRI_STDIO);
void __stdio_init(void)
{
   if (isatty(1))
      stdout->mode |= _IOLBF;
}
