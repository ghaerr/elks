/* Copyright (C) 1996 Robert de Bath <robert@debath.thenet.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */
#include <string.h>

char **__sys_errlist =0;
int __sys_nerr = 0;

char *
strerror(err)
int err;
{
   int fd;
   static char retbuf[80];
   char inbuf[256];
   int cc;
   int bufoff = 0;

   if( __sys_nerr )
   {
      if( err < 0 || err >= __sys_nerr ) goto unknown;
      return __sys_errlist[err];
   }

   if( err <= 0 ) goto unknown;	/* NB the <= allows comments in the file */
   fd = open("/lib/liberror.txt", 0);
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
	       if( p == 0 ) goto unknown;
	       while(*p == ' ') p++;
	       close(fd);
	       return p;
	    }
	    bufoff = 0;
	 }
	 else if( bufoff < sizeof(retbuf)-1 )
	    retbuf[bufoff++] = inbuf[i];
      }
   }
unknown:;
   if( fd >= 0 ) close(fd);
   strcpy(retbuf, "Unknown error ");
   strcpy(retbuf+14, itoa(err));
   return retbuf;
}
