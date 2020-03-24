#include <unistd.h>

uid_t
getegid(void)
{
   int egid;
   _getgid(&egid);
   return egid;
}
