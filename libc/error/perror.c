
#include <errno.h>

void
perror(str)
__const char * str;
{
   register char * ptr;
   if(str)
   {
      write(2, str, strlen(str));
      write(2, ": ", 2);
   }
   else write(2, "perror: ", 8);

   ptr = strerror(errno);
   write(2, ptr, strlen(ptr));
   write(2, "\n", 1);
}
