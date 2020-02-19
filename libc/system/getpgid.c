#ifdef L_getpgrp
#include <unistd.h>

int
getpgrp(void)
{
   return getpgid(0);
}
#endif
