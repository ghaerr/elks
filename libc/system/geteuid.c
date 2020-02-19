#include <unistd.h>

int
geteuid(void)
{
   int euid;
   _getuid(&euid);
   return euid;
}
