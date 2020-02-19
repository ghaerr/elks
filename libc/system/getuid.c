#include <unistd.h>

uid_t
getuid(void)
{
   int euid;
   return _getuid(&euid);
}
