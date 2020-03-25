#include <unistd.h>

uid_t
geteuid(void)
{
   int euid;
   _getuid(&euid);
   return euid;
}
