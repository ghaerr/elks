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
   int fd = -1;
   int cc;
   int bufoff = 0;
   char inbuf[256];
   static char retbuf[60];

   if( err <= 0 ) goto unknown;	/* NB the <= allows comments in the file */
   fd = open(_PATH_ERRSTRING, 0);
   if( fd < 0 ) goto unknown;

   while( (cc=read(fd, inbuf, sizeof(inbuf))) > 0 )
   {
      int i;
      for(i=0; i<cc; i++)
      {
         if( inbuf[i] == '\n' )
	 {
	    retbuf[bufoff] = '\0';
	    if( err == atoi(retbuf) )
	    {
	       char * p = strchr(retbuf, ' ');
	       if( p == 0 ) goto done;
	       while(*++p == ' ') ;
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
   strcpy(retbuf, "Unknown error ");
   strcpy(retbuf+14, itoa(err));
   return retbuf;
}
