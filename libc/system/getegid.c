#include <unistd.h>

int
getegid(void)
{
   int egid;
   _getgid(&egid);
   return egid;
}
