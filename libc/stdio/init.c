#include <stdlib.h>
#include <unistd.h>

#include "_stdio.h"

/* NOTE: ensure this destructor is lower priority (90) than
 * the atexit_do_exit (100) destructor so as to run later.
 */

#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
static DESTRUCTOR(__stdio_fini, 90);
static void __stdio_fini(void)
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
CONSTRUCTOR(__stdio_init, 90);
void __stdio_init(void)
{
   if (isatty(1))
      stdout->mode |= _IOLBF;
}
