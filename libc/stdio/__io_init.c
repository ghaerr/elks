#include <stdlib.h>
#include <unistd.h>

#include "_stdio.h"

__attribute__((destructor(99))) static void
__stdio_close_all(void)
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

__attribute__((constructor(99))) void
__io_init_vars(void)
{
   if (isatty(1))
      stdout->mode |= _IOLBF;
}
