/* Copyright (C) 1996 Robert de Bath <robert@debath.thenet.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <paths.h>

char *
strerror(int err)
{
   int i, cc, fd;
   char *p;
   size_t bufoff = 0;
   char inbuf[256];
   static char retbuf[60];

  /* NB the <= allows comments in the file */
    if( err <= 0 || (fd = open(_PATH_ERRSTRING, 0)) < 0)
        goto unknown;

   while( (cc=read(fd, inbuf, sizeof(inbuf))) > 0 )
   {
      for(i=0; i<cc; i++)
      {
         if( inbuf[i] == '\n' )
	 {
	    retbuf[bufoff] = '\0';
	    if( err == atoi(retbuf) )
	    {
	       p = retbuf;
               while (*p < 'A')
                  if (!*p++) goto done;
	       close(fd);
	       return p;
	    }
	    bufoff = 0;
	 }
	 else if( bufoff < sizeof(retbuf)-1 )
	    retbuf[bufoff++] = inbuf[i];
      }
   }
done:
   close(fd);
unknown:
   strcpy(retbuf, "Error ");
   strcpy(retbuf+6, uitoa(err));
   return retbuf;
}
