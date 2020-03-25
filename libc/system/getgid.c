#include <unistd.h>

uid_t
getgid(void)
{
   int egid;
   return _getgid(&egid);
}
